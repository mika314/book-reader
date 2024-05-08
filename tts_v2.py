import sys
import numpy as np
import torch
from TTS.api import TTS
import struct

def run_tts(model_name="tts_models/en/vctk/vits"):
    # Initialize TTS
    original_stdout = sys.stdout  # Save the original stdout
    sys.stdout = sys.stderr  # Redirect stdout to stderr

    print("TTS started\n")

    tts = TTS(model_name, gpu=True)

    for line in sys.stdin:
        # Parse line into token_id, speaker, and text
        try:
            token_id, speaker, text = line.strip().split(',', 2)
        except ValueError:
            sys.stderr.write("Invalid input format. Expected 'token_id,speaker,text'.\n")
            continue

        try:
            # Skip if text is empty or only contains whitespaces
            if not text.strip():
                continue

            # Generate speech
            wav = tts.tts(text, speaker=speaker)

            # Check if wav is a list and convert to numpy array
            if isinstance(wav, list):
                wav = np.array(wav)

            # Convert to int16_t format
            wav_int16 = (wav * 32767).astype(np.int16)

            # Create a packet
            # Magic word 'TTS\x00'
            # Size of wav data (4 bytes)
            # Token ID as string, right-padded with spaces to 10 bytes
            # wav data
            packet = struct.pack("4sI10s", b"TTS\x00", len(wav_int16.tobytes()), token_id.ljust(10).encode()) + wav_int16.tobytes()

            # Restore original stdout
            sys.stdout = original_stdout

            # Output binary packet to stdout
            sys.stdout.buffer.write(packet)

            # Redirect stdout back to stderr
            sys.stdout = sys.stderr

        except KeyboardInterrupt:
            sys.stderr.write("Exiting...\n")
            break
        except Exception as e:
            sys.stderr.write(f"An error occurred: {e}\n")

if __name__ == "__main__":
    model_name = sys.argv[1] if len(sys.argv) > 1 else "tts_models/en/vctk/vits"
    run_tts(model_name)
