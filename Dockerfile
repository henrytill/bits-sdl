FROM registry.gitlab.steamos.cloud/steamrt/scout/sdk:latest

RUN apt-get install -yqq liblua5.1-0-dev

RUN mkdir /home

RUN useradd -m build -u 1000 -U -s /bin/bash

USER build

RUN mkdir -p /home/build/sdl-bits

WORKDIR /home/build/sdl-bits

COPY --chown=build:build . /home/build/sdl-bits/

RUN make
