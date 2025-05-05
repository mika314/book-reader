

You need to update the base path to the Books library.
```
static const char *basePath = "/home/mika/Documents/belletristic/en/";
```

Download the TTS model:
```
wget https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/kokoro-en-v0_19.tar.bz2
tar xf kokoro-en-v0_19.tar.bz2
rm kokoro-en-v0_19.tar.bz2
```

Build from source Sherpa ONNX

```
https://github.com/k2-fsa/sherpa-onnx
```

