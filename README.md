**WARNING: THIS SOFTWARE IS STILL VERY MUCH EXPERIMENTAL. WHILE I DON'T EXPECT IT TO EAT YOUR HARD DRIVE, I DO EXPECT IT TO CHANGE AND YOUR FILES WILL PROBABLY SOUND LIKE SHIT IN NEWER VERSIONS OF THIS SOFTWARE.**

shADPCM: a shady codec

    cc -O3 -o enc -DENCODER shadpcm.c
    cc -O3 -o dec -DDECODER shadpcm.c

encodes/decodes 16-bit signed little-endian raw PCM

frequency is ultimately up to you

