# from diffusers import StableDiffusionXLPipeline, StableDiffusionXLImg2ImgPipeline
# import torch
# from PIL import Image
# from diffusers.utils import logging

# def get_device():
#     return "cuda" if torch.cuda.is_available() else "cpu"

# def generate_base_face(prompt, output_path):
#     """
#     Generates a base face image using the specified SDXL model.
#     """
#     device = get_device()
#     model_id = "SG161222/RealVisXL_V4.0"
#     logging.set_verbosity_error()
    
#     # Load the base pipeline with optimizations
#     pipe = StableDiffusionXLPipeline.from_pretrained(
#         model_id, 
#         torch_dtype=torch.float16,  # Use float16 instead of float32
#         variant="fp16", 
#         use_safetensors=True
#     )
#     pipe = pipe.to(device)
    
#     # Enable memory efficient attention and other optimizations
#     pipe.enable_attention_slicing()
#     # pipe.enable_model_cpu_offload()  # If you have limited VRAM
    
#     # Generate the image with fewer steps and smaller resolution
#     pipe.set_progress_bar_config(disable=True)
#     image = pipe(
#         prompt=prompt, 
#         num_inference_steps=25,  # Reduced from 25
#         height=512,  # Reduced from default 1024
#         width=512,   # Reduced from default 1024
#         guidance_scale=4  # Slightly lower guidance
#     ).images[0]
#     image.save(output_path)

# def generate_variation(base_image_path, variation_prompt, output_path):
#     """
#     Generates a variation of a face image using the specified SDXL model.
#     """
#     device = get_device()
#     model_id = "SG161222/RealVisXL_V4.0"

#     # Load the pipeline with optimizations
#     logging.set_verbosity_error()
#     pipe = StableDiffusionXLImg2ImgPipeline.from_pretrained(
#         model_id, 
#         torch_dtype=torch.float16,  # Use float16 instead of float32
#         variant="fp16", 
#         use_safetensors=True
#     )
#     pipe = pipe.to(device)
    
#     # Enable optimizations
#     pipe.enable_attention_slicing()
#     # pipe.enable_model_cpu_offload()  # If you have limited VRAM

#     init_image = Image.open(base_image_path).convert("RGB")
#     init_image = init_image.resize((512, 512))  # Reduced from 1024x1024

#     prompt = variation_prompt
#     pipe.set_progress_bar_config(disable=True)
#     image = pipe(
#         prompt=prompt, 
#         image=init_image, 
#         num_inference_steps=25,  # Reduced from 25
#         strength=0.4,  # Controls how much the image changes
#         guidance_scale=4
#     ).images[0]
#     image.save(output_path)


from diffusers import StableDiffusionXLPipeline, StableDiffusionXLImg2ImgPipeline
import torch
from PIL import Image
from diffusers.utils import logging
import os

# Disable logging
logging.set_verbosity_error()
os.environ["DIFFUSERS_VERBOSITY"] = "error"

# Global pipeline instances (one per process)
_base_pipe = None
_variation_pipe = None

def get_device():
    return "cuda" if torch.cuda.is_available() else "cpu"

def get_base_pipeline():
    global _base_pipe
    if _base_pipe is None:
        device = get_device()
        model_id = "SG161222/RealVisXL_V4.0"

        # if torch.cuda.is_available():
        #     torch.cuda.empty_cache()
        
        _base_pipe = StableDiffusionXLPipeline.from_pretrained(
            model_id, 
            torch_dtype=torch.float16,
            variant="fp16", 
            use_safetensors=True,
        )
        _base_pipe = _base_pipe.to(device)
        _base_pipe.enable_attention_slicing()
        _base_pipe.set_progress_bar_config(disable=True)
    return _base_pipe

def get_variation_pipeline():
    """Loads and caches the image-to-image pipeline."""
    global _variation_pipe
    if _variation_pipe is None:
        device = get_device()
        model_id = "SG161222/RealVisXL_V4.0"
        _variation_pipe = StableDiffusionXLImg2ImgPipeline.from_pretrained(
            model_id, torch_dtype=torch.float16, variant="fp16", use_safetensors=True
        ).to(device)
        _variation_pipe.enable_attention_slicing()
        _variation_pipe.set_progress_bar_config(disable=True)
        print(f"Process {os.getpid()}: Variation pipeline loaded on {device}.")
    return _variation_pipe

def generate_base_face(prompt, output_path, pipe):
    # pipe = get_base_pipeline()
    image = pipe(
        prompt=prompt, 
        num_inference_steps=25,
        height=512,
        width=512,
        guidance_scale=4
    ).images[0]
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    image.save(output_path)

def generate_variation(base_image_path, variation_prompt, output_path, pipe=None):
    """Generates a variation from a base image."""
    if pipe is None:
        pipe = get_variation_pipeline()
    
    init_image = Image.open(base_image_path).convert("RGB").resize((512, 512))
    
    image = pipe(
        prompt=variation_prompt, 
        image=init_image, 
        num_inference_steps=25,
        strength=0.4,
        guidance_scale=4
    ).images[0]
    image.save(output_path)
