# import pandas as pd
# import os
# import random
# import torch
# import multiprocessing as mp
# from concurrent.futures import ProcessPoolExecutor, as_completed
# from tqdm import tqdm

# def process_variation_batch(gpu_id, batch_args):
#     """
#     Processes a batch of variation tasks on a single dedicated GPU.
#     """
#     os.environ['CUDA_VISIBLE_DEVICES'] = str(gpu_id)
#     from face_generator import generate_variation, get_variation_pipeline

#     results = []
#     try:
#         pipe = get_variation_pipeline()
#     except Exception as e:
#         print(f"FATAL: GPU {gpu_id} failed to load variation pipeline: {e}")
#         for args in batch_args:
#             results.append((args[0], False, f"Pipeline load failed on GPU {gpu_id}", None))
#         return results

#     variation_prompts = [
#         "dramatic lighting",
#         "smiling",
#         "eyes closed",
#         "wearing a hat",
#         "looking up",
#         "wearing glasses",
#         "serious expression",
#         "looking down",
#         "close-up portrait",
#         "slight smirk",
#         "thoughtful gaze",
#         "three-quarter profile",
#         "raised eyebrow",
#         "soft natural light",
#         "neutral expression",
#     ]

#     for args in batch_args:
#         df_index, base_image_path, output_path, gender = args
#         try:
#             if not os.path.exists(base_image_path):
#                 raise FileNotFoundError(f"Base image not found: {base_image_path}")

#             # Create a slightly random prompt for each variation
#             variation_style = random.choice(variation_prompts)
#             prompt = f"a high-quality portrait of a {gender} person, detailed, professional headshot, {variation_style}"
            
#             generate_variation(base_image_path, prompt, output_path, pipe)
#             results.append((df_index, True, "Variation generated", output_path))
#         except Exception as e:
#             results.append((df_index, False, str(e), None))
            
#     return results

# def main():
#     # --- 1. Setup and GPU Check ---
#     if not torch.cuda.is_available() or torch.cuda.device_count() == 0:
#         print("This script requires GPUs for parallel processing.")
#         return
#     NUM_GPUS = torch.cuda.device_count()
#     print(f"Found {NUM_GPUS} GPUs for parallel variation generation.")

#     # --- 2. Data Loading and Preparation ---
#     input_csv = 'train_final_with_all_base_images_deduped.csv'
#     print(f"Loading dataset from {input_csv}...")
#     df = pd.read_csv(input_csv)

#     # --- 3. Identify Tasks and Find Base Images ---
#     print("Identifying rows that need variation images...")
#     tasks_to_process = []
    
#     # Create a fast lookup map for id -> base_image_path
#     id_to_base_path_map = df.dropna(subset=['image_path']).set_index('id')['image_path'].to_dict()
    
#     # Get all rows where image_path is NaN
#     nan_rows = df[df['image_path'].isnull()]

#     for index, row in nan_rows.iterrows():
#         person_id = row['id']
#         base_path = id_to_base_path_map.get(person_id)

#         if not base_path:
#             print(f"Warning: No base image found for ID {person_id}. Skipping row index {index}.")
#             continue

#         # Define a unique path for the new variation image
#         id_dir = os.path.dirname(base_path)
#         variation_name = f"variation_{person_id}_{index}.png"
#         output_path = os.path.join(id_dir, variation_name)
        
#         gender = 'female' if person_id % 2 == 0 else 'male'
#         tasks_to_process.append((index, base_path, output_path, gender))

#     if not tasks_to_process:
#         print("No images to generate. All image paths seem to be filled.")
#         return
        
#     print(f"Found {len(tasks_to_process)} variation images to generate.")

#     # print(tasks_to_process[0:5])

#     # --- 4. Distribute Work to GPUs ---
#     tasks_for_gpus = [[] for _ in range(NUM_GPUS)]
#     for i, args in enumerate(tasks_to_process):
#         gpu_index = i % NUM_GPUS
#         tasks_for_gpus[gpu_index].append(args)

#     # --- 5. Parallel Execution ---
#     with ProcessPoolExecutor(max_workers=NUM_GPUS) as executor:
#         future_to_gpu = {
#             executor.submit(process_variation_batch, gpu_id, task_batch): gpu_id 
#             for gpu_id, task_batch in enumerate(tasks_for_gpus) if task_batch
#         }
        
#         with tqdm(total=len(tasks_to_process), desc="Generating Variations") as pbar:
#             for future in as_completed(future_to_gpu):
#                 try:
#                     batch_results = future.result()
#                     for res_index, success, msg, new_path in batch_results:
#                         if success:
#                             df.loc[res_index, 'image_path'] = new_path
#                         else:
#                             print(f"\nFailed row index {res_index}: {msg}")
#                         pbar.update(1)
#                 except Exception as e:
#                     print(f"\nA worker process crashed: {e}")

#     # --- 6. Save Final Results ---
#     output_file = 'train_final_with_all_images.csv'
#     df.to_csv(output_file, index=False)
    
#     print(f"\n=== VARIATION GENERATION COMPLETE ===")
#     print(f"Final results with all image paths saved to: {output_file}")

# if __name__ == "__main__":
#     mp.set_start_method('spawn', force=True)
#     main()

import pandas as pd
import os
import random
import torch
import multiprocessing as mp
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm

def process_variation_batch(gpu_id, batch_args):
    """
    Processes a batch of variation tasks on a single dedicated GPU.
    """
    os.environ['CUDA_VISIBLE_DEVICES'] = str(gpu_id)
    from face_generator import generate_variation, get_variation_pipeline

    results = []
    try:
        pipe = get_variation_pipeline()
    except Exception as e:
        print(f"FATAL: GPU {gpu_id} failed to load variation pipeline: {e}")
        for args in batch_args:
            results.append((args[0], False, f"Pipeline load failed on GPU {gpu_id}", None))
        return results

    variation_prompts = [
        "dramatic lighting", "smiling", "eyes closed", "wearing a hat",
        "looking up", "wearing glasses", "serious expression", "looking down",
        "close-up portrait", "slight smirk", "thoughtful gaze",
        "three-quarter profile", "raised eyebrow", "soft natural light",
        "neutral expression",
    ]

    for args in tqdm(batch_args):
        df_index, base_image_path, output_path, gender = args
        try:
            if not os.path.exists(base_image_path):
                raise FileNotFoundError(f"Base image not found: {base_image_path}")

            variation_style = random.choice(variation_prompts)
            prompt = f"a high-quality portrait of a {gender} person, detailed, professional headshot, {variation_style}"
            
            generate_variation(base_image_path, prompt, output_path, pipe)
            results.append((df_index, True, "Variation generated", output_path))
        except Exception as e:
            results.append((df_index, False, str(e), None))
            
    return results

def main():
    # --- 1. Setup and GPU Check ---
    if not torch.cuda.is_available() or torch.cuda.device_count() == 0:
        print("This script requires GPUs for parallel processing.")
        return
    NUM_GPUS = torch.cuda.device_count()
    print(f"Found {NUM_GPUS} GPUs for parallel variation generation.")

    # --- 2. Data Loading and Preparation ---
    input_csv = 'train_final_with_all_base_images_deduped.csv'
    output_file = 'train_final_with_all_images.csv'
    print(f"Loading dataset from {input_csv}...")
    df = pd.read_csv(input_csv)

    # --- 3. Identify Tasks and Find Base Images ---
    print("Identifying rows that need variation images...")
    tasks_to_process = []
    
    id_to_base_path_map = df.dropna(subset=['image_path']).set_index('id')['image_path'].to_dict()
    nan_rows = df[df['image_path'].isnull()]

    for index, row in nan_rows.iterrows():
        person_id = row['id']
        base_path = id_to_base_path_map.get(person_id)

        if not base_path:
            print(f"Warning: No base image found for ID {person_id}. Skipping row index {index}.")
            continue

        id_dir = os.path.dirname(base_path)
        variation_name = f"variation_{person_id}_{index}.png"
        output_path = os.path.join(id_dir, variation_name)
        
        # --- RESTARTABILITY FIX ---
        # Check if the output file already exists from a previous run.
        if os.path.exists(output_path):
            df.loc[index, 'image_path'] = output_path # Update DataFrame
            continue # Skip adding this to the processing list
        # --- END FIX ---

        gender = 'female' if person_id % 2 == 0 else 'male'
        tasks_to_process.append((index, base_path, output_path, gender))

    if not tasks_to_process:
        print("No new images to generate. All variations seem to be complete.")
        df.to_csv(output_file, index=False) # Save just in case some were updated
        return
        
    print(f"Found {len(tasks_to_process)} new variation images to generate.")

    # --- 4. Distribute Work to GPUs ---
    tasks_for_gpus = [[] for _ in range(NUM_GPUS)]
    for i, args in enumerate(tasks_to_process):
        gpu_index = i % NUM_GPUS
        tasks_for_gpus[gpu_index].append(args)

    # --- 5. Parallel Execution ---
    processed_count = 0
    save_interval = 500 # Save progress every 500 images

    with ProcessPoolExecutor(max_workers=NUM_GPUS) as executor:
        future_to_gpu = {
            executor.submit(process_variation_batch, gpu_id, task_batch): gpu_id 
            for gpu_id, task_batch in enumerate(tasks_for_gpus) if task_batch
        }
        
        with tqdm(total=len(tasks_to_process), desc="Generating Variations") as pbar:
            for future in as_completed(future_to_gpu):
                try:
                    batch_results = future.result()
                    for res_index, success, msg, new_path in batch_results:
                        if success:
                            df.loc[res_index, 'image_path'] = new_path
                        else:
                            print(f"\nFailed row index {res_index}: {msg}")
                        
                        pbar.update(1)
                        processed_count += 1

                        # --- ROBUSTNESS FIX: Periodically save progress ---
                        if processed_count % save_interval == 0:
                            df.to_csv(output_file, index=False)
                            pbar.set_description(f"Progress saved... Generating")
                except Exception as e:
                    print(f"\nA worker process crashed: {e}")

    # --- 6. Save Final Results ---
    df.to_csv(output_file, index=False)
    
    print(f"\n=== VARIATION GENERATION COMPLETE ===")
    print(f"Final results with all image paths saved to: {output_file}")

if __name__ == "__main__":
    mp.set_start_method('spawn', force=True)
    main()