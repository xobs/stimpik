# Stimpik

Extract firmware from an STM32F4 that has been locked with RDP1.

This attack uses a Raspberry Pi Pico.

## Building

```
git submodule update --init
export PICO_SDK_PATH=path-to-pico-sdk
cmake -B build build
make -C build
```

## Using

Load `build/stimpik.uf2` onto a Pico. Then connect via USB.
