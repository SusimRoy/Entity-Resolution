import torch
import torch.nn as nn
from transformers import CLIPModel
import torch.nn.init as init


class MultimodalSiameseNetwork(nn.Module):
    """
    A Siamese Network that takes two sets of multimodal inputs (image + text),
    computes a combined embedding for each, and is trained with contrastive loss.
    """
    def __init__(self, model_name='openai/clip-vit-base-patch32'):
        super(MultimodalSiameseNetwork, self).__init__()
        self.clip_model = CLIPModel.from_pretrained(model_name)

        # Freeze CLIP model parameters if desired
        self.clip_model.eval()
        for param in self.clip_model.parameters():
            param.requires_grad = False

        text_embed_dim = self.clip_model.text_model.config.hidden_size
        image_embed_dim = self.clip_model.vision_model.config.hidden_size

        self.biography_projection_head = nn.Sequential(
            nn.Linear(text_embed_dim, 256)# Project to a 256-dimensional space
        )
        self.biometric_projection_head = nn.Sequential(
            nn.Linear(image_embed_dim, 256) # Project to a 256-dimensional space
        )

        self.init_weights()

    def init_weights(self):
            """
            Initialize the weights of the projection heads using Kaiming (He) initialization.
            This helps with training deep networks by keeping the variance of activations
            consistent across layers.
            """
            # Kaiming initialization for biography projection head
            init.kaiming_normal_(self.biography_projection_head[0].weight, mode='fan_out', nonlinearity='relu')
            if self.biography_projection_head[0].bias is not None:
                init.constant_(self.biography_projection_head[0].bias, 0)
            
            # Kaiming initialization for biometric projection head
            init.kaiming_normal_(self.biometric_projection_head[0].weight, mode='fan_out', nonlinearity='relu')
            if self.biometric_projection_head[0].bias is not None:
                init.constant_(self.biometric_projection_head[0].bias, 0)


    def get_embedding(self, input_ids, attention_mask, pixel_values):
        """
        Computes a single multimodal embedding for one input.
        """
        # Get separate embeddings from CLIP
        outputs = self.clip_model(
            input_ids=input_ids,
            attention_mask=attention_mask,
            pixel_values=pixel_values,
            return_dict=True
        )

        image_embedding = outputs.vision_model_output.pooler_output
        text_embedding = outputs.text_model_output.pooler_output

        text_embedding = self.biography_projection_head(text_embedding)

        image_embedding = self.biometric_projection_head(image_embedding)
        text_embedding = nn.functional.normalize(text_embedding, p=2, dim=1)
        image_embedding = nn.functional.normalize(image_embedding, p=2, dim=1)

        combined_embedding = torch.cat((image_embedding, text_embedding), dim=1)

        return combined_embedding

    def forward(self, batch):
        """
        Process a batch of paired data.
        """
        # Get embedding for the first item in the pair
        embedding1 = self.get_embedding(
            input_ids=batch['input_ids_1'],
            attention_mask=batch['attention_mask_1'],
            pixel_values=batch['pixel_values_1']
        )

        # Get embedding for the second item in the pair
        embedding2 = self.get_embedding(
            input_ids=batch['input_ids_2'],
            attention_mask=batch['attention_mask_2'],
            pixel_values=batch['pixel_values_2']
        )

        return embedding1, embedding2


class ContrastiveLoss(nn.Module):
    """
    Contrastive loss function.
    Based on: http://yann.lecun.com/exdb/publis/pdf/hadsell-chopra-lecun-06.pdf
    """
    def __init__(self, margin=2.0):
        super(ContrastiveLoss, self).__init__()
        self.margin = margin

    def forward(self, output1, output2, label):
        euclidean_distance = nn.functional.pairwise_distance(output1, output2, keepdim = True)
        loss_contrastive = torch.mean((label) * torch.pow(euclidean_distance, 2) +
                                      (1-label) * torch.pow(torch.clamp(self.margin - euclidean_distance, min=0.0), 2))

        return loss_contrastive