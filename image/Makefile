build:
	nix build .#qcow
	install -m644 result/nixos.qcow2 nixos.qcow2
	rm -rf result

DOCKER ?= podman

container-build:
	$(DOCKER) build . -t nix-image-builder
	$(DOCKER) run -it --rm -v .:/build --privileged \
		-v /dev/kvm:/dev/kvm:rw \
		localhost/nix-image-builder:latest \
		make build

.PHONY: build container-build
