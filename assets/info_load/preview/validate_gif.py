from PIL import Image
p=r"C:\Users\wolff\Documents\SDKVita\VoidStranger\assets\info_load\preview\missing_files_scene_preview.gif"
im=Image.open(p)
print(f"DIMENSIONS={im.width}x{im.height}")
print(f"FRAMES={getattr(im,'n_frames',1)}")
print(f"DURATION_MS={im.info.get('duration')}")
