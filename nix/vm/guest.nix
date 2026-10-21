{ inputs, ... }:
{
  flake.modules.nixos.titbit-vm =
    {
      pkgs,
      lib,
      config,
      ...
    }:
    let
      engine = inputs.self.packages.x86_64-linux.titbit;
      play = pkgs.writeShellScript "titbit-session" ''
        export LIBGL_ALWAYS_SOFTWARE=1
        args=()
        if [ -f /tmp/shared/args ]; then
          mapfile -d "" -t args < /tmp/shared/args
        fi
        ${engine}/bin/titbit --data /home/titbit/game "''${args[@]}"
        status=$?
        if [ "$status" -ne 0 ]; then
          printf '\nGame exited with status %s. Press Enter to close the VM.\n' "$status"
          read -r _
        fi
        ${pkgs.systemd}/bin/systemctl poweroff
      '';
    in
    {
      imports = [ (inputs.nixpkgs + "/nixos/modules/virtualisation/qemu-vm.nix") ];
      system.stateVersion = "26.05";
      networking.hostName = "titbit";
      nix.enable = false;
      documentation.enable = false;
      users.users.titbit = {
        isNormalUser = true;
        uid = 1000;
        initialPassword = "";
        extraGroups = [
          "audio"
          "video"
        ];
      };
      hardware.graphics = {
        enable = true;
        enable32Bit = true;
      };
      services = {
        pipewire.enable = false;
        pulseaudio.enable = true;
        xserver = {
          enable = true;
          videoDrivers = [ "modesetting" ];
          displayManager.lightdm = {
            enable = true;
            greeter.enable = false;
          };
          desktopManager.session = [
            {
              name = "titbit";
              bgSupport = true;
              start = ''
                ${pkgs.openbox}/bin/openbox --sm-disable &
                ${pkgs.xterm}/bin/xterm -T "Titbit" -fullscreen -e ${play} &
                waitPID=$!
              '';
            }
          ];
        };
        displayManager = {
          autoLogin = {
            enable = true;
            user = "titbit";
          };
          defaultSession = "titbit";
        };
      };
      # Present files owned by the host's UID (501 on many Macs) to UID 1000.
      systemd.services.titbit-data = {
        wantedBy = [ "multi-user.target" ];
        before = [ "display-manager.service" ];
        after = [ "mnt-game.mount" ];
        requires = [ "mnt-game.mount" ];
        serviceConfig.Type = "forking";
        script = ''
          mkdir -p /home/titbit/game
          ${pkgs.bindfs}/bin/bindfs --force-user=titbit --force-group=users \
            --perms=u+rwX -o allow_other --chmod-ignore --chown-ignore --chgrp-ignore \
            /mnt/game /home/titbit/game
        '';
      };
      programs.fuse.userAllowOther = true;
      virtualisation = {
        memorySize = 2048;
        cores = 2;
        diskImage = null;
        writableStore = false;
        fileSystems."/nix/.ro-store".options = [ "ro" ];
        useHostCerts = false;
        sharedDirectories.game = {
          source = ''"$TITBIT_GAME"'';
          target = "/mnt/game";
          securityModel = "none";
        };
        # Only the game directory and the read-only Nix store are needed.
        qemu.networkingOptions = lib.mkForce [ "-nic none" ];
        qemu.options = [
          "-vga virtio"
          "-device intel-hda"
          "-device hda-duplex,audiodev=audio0"
          "-audiodev \${TITBIT_VM_AUDIO:-${
            if config.virtualisation.host.pkgs.stdenv.hostPlatform.isDarwin then "coreaudio" else "pa"
          }},id=audio0"
        ];
      };
    };
}
