FROM devkitpro/devkitarm:20260221

# Install additional dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        git \
        vim \
        bear \
        && rm -rf /var/lib/apt/lists/*

# Set environment variables
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH="${DEVKITARM}/bin:${DEVKITPRO}/tools/bin:${DEVKITPRO}/portlibs/3ds/bin:${PATH}"

# bannertool and makerom (needed for `make cia`/`make debug-cia`) aren't in
# devkitPro's pacman repos - they're standalone GitHub releases.
RUN BANNERTOOL_URL=$(curl -sSL https://api.github.com/repos/carstene1ns/3ds-bannertool/releases/latest | \
        grep -o 'https://[^"]*-linux\.tar\.gz') && \
    curl -sSL -o /tmp/bannertool.tar.gz "$BANNERTOOL_URL" && \
    tar xzf /tmp/bannertool.tar.gz -C /tmp && \
    install -m 755 /tmp/bannertool-*-linux/bannertool /opt/devkitpro/tools/bin/bannertool && \
    rm -rf /tmp/bannertool.tar.gz /tmp/bannertool-*-linux

RUN MAKEROM_URL=$(curl -sSL https://api.github.com/repos/3DSGuy/Project_CTR/releases/latest | \
        grep -o 'https://[^"]*ubuntu_x86_64\.zip') && \
    curl -sSL -o /tmp/makerom.zip "$MAKEROM_URL" && \
    unzip -oq /tmp/makerom.zip -d /opt/devkitpro/tools/bin && \
    chmod 755 /opt/devkitpro/tools/bin/makerom && \
    rm /tmp/makerom.zip

# Set working directory
WORKDIR /project

# Default command
CMD ["/bin/bash"]
