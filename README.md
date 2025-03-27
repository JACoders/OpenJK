## Overview

Char36 OpenJK fork. See [OpenJK README](https://github.com/JACoders/OpenJK/blob/master/README.md) for info about it.

## Development

1. Download JKA game files and put them pk3s into /ci/assets/
2. Build and run Dockerfile:

    ```bash
    docker build -t jka_dedicated .
    docker run -p 1337:29070/udp jka_dedicated
    ```