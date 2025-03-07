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
          ({ config, pkgs, ... }:
            let
              checkerServer = pkgs.stdenv.mkDerivation {
                name = "checker-server";
                src = ./checker_server.py;
                phases = [ "installPhase" ];
                installPhase = ''
                  install -Dm755 $src $out/bin/checker_server
                '';
              };
            in
            {
              environment.systemPackages = [ pkgs.python3 checkerServer ];

              systemd.services.checker-server = {
                wantedBy = [ "multi-user.target" ];
                serviceConfig = {
                  ExecStart = "${pkgs.python3}/bin/python3 ${checkerServer}/bin/checker_server";
                  Restart = "on-failure";
                };
              };

              networking.firewall.enable = false;

              users.users.root.password = "root";

              system.stateVersion = "24.11";
            }
          )
        ];
      };
    };
  };
}
