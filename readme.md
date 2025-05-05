This is a book reader app for reading books. It can also read books aloud using TTS technology.

I put it together for personal use; I need a simple book reader that can scroll, remember the last read position, and have decent quality TTS.

The build steps are not simple.

You need to update the base path to the Books library.
```
static const char *basePath = "/home/mika/Documents/belletristic/en/";
```

### **Building Instructions on Ubuntu 22.04**

1. Build from source Sherpa ONNX

Go to <https://github.com/k2-fsa/sherpa-onnx> and follow the build and install instructions. For some reason, the install `.pc` file is in the incorrect directory; manually move it to the correct directory:

```bash
sudo mv /usr/local/sherpa-onnx.pc /usr/lib/x86_64-linux-gnu/pkgconfig/
```

2. **Install Dependencies**
   ```bash
   sudo apt update
   sudo apt install -y \
   clang \
   git \
   libsdl2-dev \
   pkg-config
   ```

3. **Install and Build `coddle` (Build Tool)**
   ```bash
   git clone https://github.com/coddle-cpp/coddle.git
   cd coddle
   ./build.sh
   sudo ./deploy.sh
   cd ..
   ```

4. **Clone and Build Screen Cast**
   ```bash
   git clone https://github.com/mika314/book-reader.git
   cd book-reader
   make
   ```


Download the TTS model:
```
wget https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/kokoro-en-v0_19.tar.bz2
tar xf kokoro-en-v0_19.tar.bz2
rm kokoro-en-v0_19.tar.bz2
```
