# --- Constants ---
# Define constants using descriptive uppercase names
# The size is in bytes, so a comment clarifies the hexadecimal value (32 KB)
BOOTLOADER_SIZE_BYTES = 0x8000  # 32 KB
BOOTLOADER_FILE_NAME = "firmware-bootloader.bin"

def pad_bootloader_file(file_path: str, target_size: int, pad_byte: int = 0xFF):
    """
    Pads a binary file to a specific target size by appending a padding byte.
    The file is read, padding is calculated, and the file is overwritten
    with the original content plus the padding.

    Args:
        file_path (str): The path to the binary file to pad.
        target_size (int): The desired final size of the file in bytes.
        pad_byte (int): The byte value to use for padding (default is 0xFF).
    """
    try:
        # Read the current file content
        with open(file_path, "rb") as f:
            raw_data = f.read()

        current_size = len(raw_data)

        # Check if padding is necessary
        if current_size >= target_size:
            print(f"File '{file_path}' is already {current_size} bytes.")
            if current_size > target_size:
                print(f"Warning: File size exceeds target size of {target_size} bytes. No padding applied.")
            return

        # Calculate padding and generate padding bytes
        bytes_to_pad = target_size - current_size
        
        # Use a simpler way to generate a sequence of repeated bytes
        padding = bytes([pad_byte]) * bytes_to_pad
        
        # Overwrite the file with original data + padding
        print(f"Padding '{file_path}' from {current_size} bytes to {target_size} bytes (+{bytes_to_pad} bytes).")
        with open(file_path, "wb") as f:
            f.write(raw_data + padding)
            
    except FileNotFoundError:
        print(f"Error: File not found at '{file_path}'")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

# --- Execution ---
if __name__ == "__main__":
    pad_bootloader_file(BOOTLOADER_FILE_NAME, BOOTLOADER_SIZE_BYTES)
