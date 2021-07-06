This demo runs on the [milliLearn dev board](https://github.com/johnMamish/milliLearn-boards/tree/master/ice40).

It records audio from the on-board [sph0645lm4h](https://cdn-shop.adafruit.com/product-files/3421/i2S+Datasheet.PDF) microphones and sends the audio over serial as 8-bit PCM.

I'd like to support 16-bit PCM soon once I have a good way to frame the bytes and avoid endian-ness swapping issues. I could go about doing this by...

  Just picking a magic sequence of bytes, hope that that sequence doesn't appear in any real data, and use that as a "sync" sequence.

But then to make it less likely to run into uses, I could...

    Add an escape sequence for whenever that sequence happens to appear in real data.