FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV TZ=Etc/UTC
ENV PIPX_HOME=/opt/pipx
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PATH=/usr/local/bin:/opt/pipx/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN apt-get update && apt-get install -y --no-install-recommends \
    bc \
    binutils-riscv64-linux-gnu \
    binutils-riscv64-unknown-elf \
    bison \
    build-essential \
    ca-certificates \
    ccache \
    cmake \
    cpio \
    device-tree-compiler \
    file \
    flex \
    g++-riscv64-linux-gnu \
    gcc-riscv64-linux-gnu \
    gcc-riscv64-unknown-elf \
    git \
    gzip \
    libelf-dev \
    libncurses-dev \
    libssl-dev \
    make \
    ninja-build \
    patch \
    perl \
    pipx \
    python3 \
    python3-pip \
    python3-venv \
    qemu-system-misc \
    rsync \
    sudo \
    tzdata \
    unzip \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN pipx install west && pipx inject west pyelftools

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

RUN set -eux; \
    existing_group="$(getent group "${USER_GID}" | cut -d: -f1 || true)"; \
    if [ -z "${existing_group}" ]; then \
        groupadd --gid "${USER_GID}" "${USERNAME}"; \
    fi; \
    existing_user="$(getent passwd "${USER_UID}" | cut -d: -f1 || true)"; \
    if [ -n "${existing_user}" ] && [ "${existing_user}" != "${USERNAME}" ]; then \
        usermod --login "${USERNAME}" --move-home --home "/home/${USERNAME}" "${existing_user}"; \
    elif [ -z "${existing_user}" ]; then \
        useradd --uid "${USER_UID}" --gid "${USER_GID}" -m -s /bin/bash "${USERNAME}"; \
    fi; \
    usermod --uid "${USER_UID}" --gid "${USER_GID}" -d "/home/${USERNAME}" "${USERNAME}"; \
    mkdir -p "/home/${USERNAME}"; \
    chown "${USER_UID}:${USER_GID}" "/home/${USERNAME}"; \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >/etc/sudoers.d/"${USERNAME}"; \
    chmod 0440 /etc/sudoers.d/"${USERNAME}"

WORKDIR /workspace

COPY docker/entrypoint.sh /usr/local/bin/risc5-entrypoint
RUN chmod 0755 /usr/local/bin/risc5-entrypoint

USER ${USERNAME}

ENTRYPOINT ["risc5-entrypoint"]
CMD ["bash"]
