FROM nixos/nix:2.26.3

RUN echo "extra-experimental-features = nix-command flakes" >>/etc/nix/nix.conf && \
	echo "system-features = benchmark big-parallel nixos-test uid-range kvm" >>/etc/nix/nix.conf

RUN nix profile install nixpkgs#gnumake

WORKDIR /build
