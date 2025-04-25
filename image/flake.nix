{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-24.11";
    nixos-generators = {
      url = "github:nix-community/nixos-generators";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, nixos-generators, ... }@inputs: {
    packages.x86_64-linux = {
      qcow = nixos-generators.nixosGenerate {
        system = "x86_64-linux";
        format = "qcow";
        #specialArgs = { inherit inputs; };
        modules = [
          "${nixpkgs}/nixos/modules/profiles/minimal.nix"
          "${nixpkgs}/nixos/modules/profiles/headless.nix"
          ({ config, pkgs, lib, ... }:
            let
              magicDirectory = "/kernel-sources";

              checkerServer = pkgs.stdenv.mkDerivation {
                name = "checker-server";
                src = ./checker_server.py;
                phases = [ "installPhase" "postFixup" ];

                buildInputs = with pkgs; [
                  nix
                  python3
                  gnumake
                  bash
                  gawk
                  kmod
                  util-linux
                  linux.dev
                ] ++ pkgs.linux.nativeBuildInputs ++ pkgs.linux.depsBuildBuild;

                nativeBuildInputs = [ pkgs.makeWrapper ];

                installPhase = ''
                  install -Dm755 $src $out/bin/checker_server
                '';

                postFixup = ''
                  wrapProgram $out/bin/checker_server \
                    --prefix PATH : ${pkgs.lib.makeBinPath (with pkgs; [
                      nix
                      python3
                      gnumake
                      bash
                      gawk
                      kmod
                      util-linux
                      linux.dev
                    ] ++ pkgs.linux.nativeBuildInputs ++ pkgs.linux.depsBuildBuild)}
                '';
              };
            in
            {
              environment.systemPackages = [
                checkerServer
              ];

              # Directly link linux.dev to a fixed directory
              system.activationScripts.linkLinuxDev = {
                text = ''
                  ln -sfn ${pkgs.linux.dev} ${magicDirectory}
                '';
                deps = [ ];
              };

              systemd.services.checker-server = {
                wantedBy = [ "multi-user.target" ];
                serviceConfig = {
                  ExecStart = "${checkerServer}/bin/checker_server";
                  Restart = "on-failure";
                };
              };

              networking.firewall.enable = false;

              users.users.root.password = "root";

              nix = {
                # make nix3 commands consistent with the flake
                registry = lib.mapAttrs (_: value: { flake = value; }) inputs;

                settings.experimental-features = "nix-command flakes";

                # We don't actually need Nix on our system
                # because everything is done at build-time
                enable = false;
              };

              system.stateVersion = "24.11";
            }
          )
        ];
      };
    };
  };
}
