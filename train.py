import torch
from torch.utils.data import DataLoader
from transformers import CLIPProcessor
from torch.optim import AdamW, SGD
from tqdm import tqdm
import mlflow
import numpy as np
import mlflow.pytorch
from sklearn.metrics import accuracy_score, roc_auc_score, precision_recall_fscore_support

from data.dataset import EntityDataset
from model import MultimodalSiameseNetwork, ContrastiveLoss

def main():
    # --- 1. Configuration ---
    DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
    JSONL_PATH = '/home/csgrad/susimmuk/authentication/vlm_prompts_all.jsonl'
    MODEL_NAME = 'openai/clip-vit-base-patch32'
    BATCH_SIZE = 256
    LEARNING_RATE = 1e-5
    EPOCHS = 5
    MARGIN = 2.0 # For the contrastive loss

    print(f"Using device: {DEVICE}")

    # --- 2. Load Model and Processor ---
    processor = CLIPProcessor.from_pretrained(MODEL_NAME)
    model = MultimodalSiameseNetwork(model_name=MODEL_NAME).to(DEVICE)
    print(model)

    total_params = sum(p.numel() for p in model.parameters())
    trainable_params = sum(p.numel() for p in model.parameters() if p.requires_grad)
    print("--- Model Summary ---")
    print(f"Total Parameters: {total_params:,}")
    print(f"Trainable Parameters: {trainable_params:,}")
    print(f"Non-Trainable Parameters: {total_params - trainable_params:,}")
    print("---------------------")

    # --- 3. Prepare Dataset ---
    print("Loading dataset...")
    dataset = EntityDataset(jsonl_path=JSONL_PATH, clip_processor=processor)
    train_loader = DataLoader(dataset, batch_size=BATCH_SIZE, shuffle=True)
    print("Dataset loaded.")

    # --- 4. Setup Training --- 
    criterion = ContrastiveLoss(margin=MARGIN)
    optimizer = SGD(model.parameters(), lr=LEARNING_RATE)

    # --- 5. MLflow Experiment Setup ---
    mlflow.set_experiment("Siamese Authentication")

    with mlflow.start_run() as run:
        print(f"MLflow Run ID: {run.info.run_id}")
        
        # Log hyperparameters
        params = {
            "learning_rate": LEARNING_RATE,
            "epochs": EPOCHS,
            "batch_size": BATCH_SIZE,
            "margin": MARGIN,
            "model_name": MODEL_NAME,
        }
        mlflow.log_params(params)

        # --- 6. Training Loop ---
        print("Starting training...")
        model.train()

        global_step = 0
        for epoch in range(EPOCHS):
            total_loss = 0
            all_labels = []
            all_distances = []
            
            progress_bar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{EPOCHS}")
            
            for batch in progress_bar:
                for key in batch:
                    batch[key] = batch[key].to(DEVICE)
                
                optimizer.zero_grad()
                
                embedding1, embedding2 = model(batch)
                
                # Calculate distance for metrics
                # embedding1 = torch.nn.functional.normalize(embedding1, p=2, dim=1)
                # embedding2 = torch.nn.functional.normalize(embedding2, p=2, dim=1)
                euclidean_distance = torch.nn.functional.pairwise_distance(embedding1, embedding2)
                
                loss = criterion(embedding1, embedding2, batch['label'])
                
                loss.backward()
                optimizer.step()
                
                total_loss += loss.item()
                
                # Store labels and distances for epoch-level metrics
                all_labels.extend(batch['label'].cpu().numpy())
                all_distances.extend(euclidean_distance.detach().cpu().numpy())
                
                # Log batch loss
                mlflow.log_metric("batch_loss", loss.item(), step=global_step)
                global_step += 1
                
                progress_bar.set_postfix({'loss': loss.item()})

            avg_loss = total_loss / len(train_loader)
            print(f"Epoch {epoch+1} finished. Average Loss: {avg_loss:.4f}")
            
            # --- Calculate and Log Epoch-Level Metrics ---
            all_labels = np.array(all_labels)
            all_distances = np.array(all_distances)

            # Define a threshold to calculate accuracy (a common choice is margin/2)
            threshold = 1.0
            predictions = (all_distances < threshold).astype(int)

            # Calculate metrics
            accuracy = accuracy_score(all_labels, predictions)
            auc = roc_auc_score(all_labels, 1 - all_distances) # AUC expects scores where higher is better for the positive class
            precision, recall, f1, _ = precision_recall_fscore_support(all_labels, predictions, average='binary')
            
            # Calculate mean distances
            pos_dist_mean = all_distances[all_labels == 1].mean()
            neg_dist_mean = all_distances[all_labels == 0].mean()

            # Log epoch-level metrics
            mlflow.log_metric("avg_loss", avg_loss, step=epoch)
            mlflow.log_metric("accuracy", accuracy, step=epoch)
            mlflow.log_metric("auc", auc, step=epoch)
            mlflow.log_metric("precision", precision, step=epoch)
            mlflow.log_metric("recall", recall, step=epoch)
            mlflow.log_metric("f1_score", f1, step=epoch)
            mlflow.log_metric("positive_distance_mean", pos_dist_mean, step=epoch)
            mlflow.log_metric("negative_distance_mean", neg_dist_mean, step=epoch)

            print(f"Accuracy: {accuracy:.4f}, AUC: {auc:.4f}, F1: {f1:.4f}")
            print(f"Mean Distance (Pos): {pos_dist_mean:.4f}, Mean Distance (Neg): {neg_dist_mean:.4f}")


        # Log the final model
        mlflow.pytorch.log_model(model, "model")
        print("Model saved to MLflow.")

    print("Training complete.")

if __name__ == '__main__':
    main()