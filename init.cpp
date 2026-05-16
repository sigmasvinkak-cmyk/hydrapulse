// -------------------------------------------------
// init.cpp – Hydrapulse init (PID 1) + installer
// -------------------------------------------------
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

// ============ ANSI COLORS ============
#define RST  "\033[0m"
#define BOLD "\033[1m"
#define DIM  "\033[2m"
#define RED  "\033[1;31m"
#define GRN  "\033[1;32m"
#define YEL  "\033[1;33m"
#define BLU  "\033[1;34m"
#define MAG  "\033[1;35m"
#define CYN  "\033[1;36m"
#define WHT  "\033[1;37m"

// ============ HELPERS ============

int run(const std::string& cmd) {
    return system(cmd.c_str());
}

std::string run_get(const std::string& cmd) {
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (p) {
        char buf[256];
        while (fgets(buf, sizeof(buf), p)) out += buf;
        pclose(p);
    }
    // trim trailing newline
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

std::string human_size(long long bytes) {
    const char* u[] = {"B","KB","MB","GB","TB"};
    int i = 0;
    double s = bytes;
    while (s >= 1024 && i < 4) { s /= 1024; i++; }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f %s", s, u[i]);
    return buf;
}

// ============ DISK DETECTION ============

struct Disk {
    std::string name, path, size_h;
    long long size_b;
};

std::vector<Disk> detect_disks() {
    std::vector<Disk> v;
    std::ifstream f("/proc/partitions");
    std::string line;
    std::getline(f, line); // header
    std::getline(f, line); // blank
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        int maj, min; long long blocks; std::string name;
        if (ss >> maj >> min >> blocks >> name) {
            if (min == 0
                && name.find("ram")  == std::string::npos
                && name.find("loop") == std::string::npos) {
                Disk d;
                d.name = name;
                d.path = "/dev/" + name;
                d.size_b = blocks * 1024;
                d.size_h = human_size(d.size_b);
                v.push_back(d);
            }
        }
    }
    return v;
}

// ============ DESKTOP ENVIRONMENTS ============

struct DE { std::string id, name; };

std::vector<DE> get_des() {
    return {
        {"kde-plasma",  "KDE Plasma"},
        {"gnome",       "GNOME"},
        {"xfce",        "XFCE"},
        {"cinnamon",    "Cinnamon"},
        {"mate",        "MATE"},
        {"lxqt",        "LXQt"},
        {"budgie",      "Budgie"},
        {"deepin",      "Deepin Desktop"},
        {"i3wm",        "i3 Window Manager"},
        {"hyprland",    "Hyprland"},
        {"none",        "None (minimal CLI)"}
    };
}

// ============ HYDRAINSTALL ============

void hydrainstall() {
    std::string input;

    std::cout << "\n";
    std::cout << CYN "  ╔══════════════════════════════════════╗\n";
    std::cout <<      "  ║        HYDRAINSTALL  v0.1           ║\n";
    std::cout <<      "  ║     Hydrapulse System Installer     ║\n";
    std::cout <<      "  ╚══════════════════════════════════════╝\n" RST;

    // ---- 1/5  DISK ----
    std::cout << "\n" YEL "  [1/5] " WHT "Disk Selection\n" RST;
    std::cout << DIM "  ─────────────────────────────\n" RST;

    auto disks = detect_disks();
    if (disks.empty()) {
        std::cout << RED "  Error: no disks found!\n" RST;
        return;
    }
    for (size_t i = 0; i < disks.size(); i++)
        std::cout << GRN "    " << i+1 << ") " RST
                  << disks[i].path << " — " CYN << disks[i].size_h << RST "\n";

    std::cout << "\n  Select disk [1-" << disks.size() << "]: ";
    std::getline(std::cin, input);
    int di = 0;
    try { di = std::stoi(input) - 1; } catch(...) {}
    if (di < 0 || di >= (int)disks.size()) {
        std::cout << RED "  Invalid. Aborted.\n" RST; return;
    }
    Disk& disk = disks[di];

    std::cout << "\n" RED "  ⚠ ALL data on " << disk.path
              << " (" << disk.size_h << ") will be ERASED!\n" RST;
    std::cout << "  Type 'yes' to continue: ";
    std::getline(std::cin, input);
    if (input != "yes") { std::cout << YEL "  Aborted.\n" RST; return; }

    // ---- 2/5  DESKTOP ----
    std::cout << "\n" YEL "  [2/5] " WHT "Desktop Environment\n" RST;
    std::cout << DIM "  ─────────────────────────────\n" RST;

    auto des = get_des();
    for (size_t i = 0; i < des.size(); i++)
        std::cout << GRN "    " << i+1 << ") " RST << des[i].name << "\n";

    std::cout << "\n  Select DE [1-" << des.size() << "]: ";
    std::getline(std::cin, input);
    int ei = 0;
    try { ei = std::stoi(input) - 1; } catch(...) {}
    if (ei < 0 || ei >= (int)des.size()) {
        std::cout << RED "  Invalid. Aborted.\n" RST; return;
    }
    std::string de_id   = des[ei].id;
    std::string de_name = des[ei].name;

    // ---- 3/5  PROFILE ----
    std::cout << "\n" YEL "  [3/5] " WHT "User Profile\n" RST;
    std::cout << DIM "  ─────────────────────────────\n" RST;

    std::cout << "  Use root for all? [true/false]: ";
    std::getline(std::cin, input);
    bool root_all = (input == "true" || input == "yes");

    std::string username = "root";
    std::string password;

    if (!root_all) {
        std::cout << "  Username: ";
        std::getline(std::cin, username);
        if (username.empty()) {
            std::cout << RED "  Empty name. Aborted.\n" RST; return;
        }
    }
    std::cout << "  Password: ";
    std::getline(std::cin, password);
    if (password.empty()) {
        std::cout << RED "  Empty password. Aborted.\n" RST; return;
    }

    // ---- 4/5  CONFIRM ----
    std::cout << "\n" YEL "  [4/5] " WHT "Confirmation\n" RST;
    std::cout << DIM "  ─────────────────────────────\n" RST;
    std::cout << "  Disk:     " CYN << disk.path << " (" << disk.size_h << ")" RST "\n";
    std::cout << "  Format:   " CYN "ext4" RST "\n";
    std::cout << "  Desktop:  " CYN << de_name << RST "\n";
    std::cout << "  Root all: " CYN << (root_all?"true":"false") << RST "\n";
    std::cout << "  User:     " CYN << username << RST "\n";

    std::cout << "\n  Proceed? [yes/no]: ";
    std::getline(std::cin, input);
    if (input != "yes") { std::cout << YEL "  Aborted.\n" RST; return; }

    // ---- 5/5  INSTALL ----
    std::cout << "\n" YEL "  [5/5] " WHT "Installing...\n" RST;
    std::cout << DIM "  ─────────────────────────────\n" RST;

    std::string p1 = disk.path + "1";

    // partition
    std::cout << "  " CYN "▸" RST " Partitioning " << disk.path << "...";
    std::cout.flush();
    std::string fc = "echo -e 'o\\nn\\np\\n1\\n\\n\\nw' | fdisk "
                     + disk.path + " >/dev/null 2>&1";
    if (run(fc) != 0) {
        std::cout << RED " FAILED\n" RST; return;
    }
    // give kernel time to re-read partition table
    sleep(1);
    run("partprobe " + disk.path + " 2>/dev/null");
    sleep(1);
    std::cout << GRN " done\n" RST;

    // format
    std::cout << "  " CYN "▸" RST " Formatting as ext4...";
    std::cout.flush();
    if (run("mkfs.ext4 -F " + p1 + " >/dev/null 2>&1") != 0) {
        if (run("mke2fs -t ext4 -F " + p1 + " >/dev/null 2>&1") != 0) {
            std::cout << RED " FAILED\n" RST; return;
        }
    }
    std::cout << GRN " done\n" RST;

    // mount
    std::cout << "  " CYN "▸" RST " Mounting filesystem...";
    std::cout.flush();
    mkdir("/mnt/hydra", 0755);
    if (mount(p1.c_str(), "/mnt/hydra", "ext4", 0, "") != 0) {
        run("mount -t ext4 " + p1 + " /mnt/hydra 2>/dev/null");
    }
    std::cout << GRN " done\n" RST;

    // directory tree
    std::cout << "  " CYN "▸" RST " Creating directories...";
    std::cout.flush();
    const char* dirs[] = {
        "/mnt/hydra/boot","/mnt/hydra/boot/grub",
        "/mnt/hydra/bin","/mnt/hydra/sbin",
        "/mnt/hydra/etc","/mnt/hydra/etc/hydrapulse",
        "/mnt/hydra/dev","/mnt/hydra/proc","/mnt/hydra/sys",
        "/mnt/hydra/tmp","/mnt/hydra/var","/mnt/hydra/var/log",
        "/mnt/hydra/home","/mnt/hydra/root",
        "/mnt/hydra/usr","/mnt/hydra/usr/bin","/mnt/hydra/usr/lib"
    };
    for (auto d : dirs) mkdir(d, 0755);
    if (!root_all)
        mkdir(("/mnt/hydra/home/" + username).c_str(), 0755);
    std::cout << GRN " done\n" RST;

    // copy system
    std::cout << "  " CYN "▸" RST " Copying system files...";
    std::cout.flush();
    run("cp /init /mnt/hydra/sbin/init 2>/dev/null || true");
    run("cp /bin/busybox /mnt/hydra/bin/busybox 2>/dev/null || true");
    run("/mnt/hydra/bin/busybox --install -s /mnt/hydra/bin/ 2>/dev/null || true");
    std::cout << GRN " done\n" RST;

    // config files
    std::cout << "  " CYN "▸" RST " Writing configuration...";
    std::cout.flush();
    {
        std::ofstream f("/mnt/hydra/etc/hydrapulse/system.conf");
        f << "HYDRA_VERSION=0.2\n"
          << "DESKTOP_ENV=" << de_id << "\n"
          << "ROOT_ALL=" << (root_all?"true":"false") << "\n"
          << "DEFAULT_USER=" << username << "\n";
    }
    {
        std::ofstream f("/mnt/hydra/etc/fstab");
        f << p1 << "  /  ext4  defaults  0  1\n";
    }
    {
        std::ofstream f("/mnt/hydra/etc/hostname");
        f << "hydrapulse\n";
    }
    {
        std::ofstream f("/mnt/hydra/etc/passwd");
        f << "root:x:0:0:root:/root:/bin/sh\n";
        if (!root_all && username != "root")
            f << username << ":x:1000:1000:" << username
              << ":/home/" << username << ":/bin/sh\n";
    }
    std::cout << GRN " done\n" RST;

    // bootloader
    std::cout << "  " CYN "▸" RST " Installing bootloader...";
    std::cout.flush();
    {
        std::ofstream f("/mnt/hydra/boot/grub/grub.cfg");
        f << "set timeout=5\nset default=0\n\n"
          << "menuentry \"Hydrapulse\" {\n"
          << "    linux /boot/vmlinuz root=" << p1 << " ro quiet init=/sbin/init\n"
          << "    initrd /boot/initramfs.cpio.gz\n"
          << "}\n";
    }
    run("grub-install --target=i386-pc --boot-directory=/mnt/hydra/boot "
        + disk.path + " 2>/dev/null || true");
    std::cout << GRN " done\n" RST;

    // unmount
    std::cout << "  " CYN "▸" RST " Unmounting...";
    std::cout.flush();
    sync();
    umount("/mnt/hydra");
    std::cout << GRN " done\n" RST;

    std::cout << "\n" GRN "  ✓ Installation complete!" RST "\n";
    std::cout << "  Type " CYN "reboot" RST " to start Hydrapulse.\n\n";
}

// ============ SPLASH SCREEN ============

void print_splash() {
    std::cout << CYN R"(
    ╦ ╦╦ ╦╔╦╗╦═╗╔═╗╔═╗╦ ╦╦  ╔═╗╔═╗
    ╠═╣╚╦╝ ║║╠╦╝╠═╣╠═╝║ ║║  ╚═╗║╣
    ╩ ╩ ╩ ═╩╝╩╚═╩ ╩╩  ╚═╝╩═╝╚═╝╚═╝
)" RST;
    std::cout << DIM "    Hydrapulse OS v0.2 beta\n" RST;
    std::cout << DIM "    Type 'help' for commands\n\n" RST;
}

// ============ SYSTEM INIT ============

void init_system() {
    mkdir("/proc", 0755);
    mkdir("/sys",  0755);
    mkdir("/dev",  0755);
    mkdir("/tmp",  0755);
    mkdir("/mnt",  0755);
    mount("proc",    "/proc", "proc",     0, "");
    mount("sysfs",   "/sys",  "sysfs",    0, "");
    mount("devtmpfs","/dev",  "devtmpfs", 0, "");
    mount("tmpfs",   "/tmp",  "tmpfs",    0, "");
}

// ============ SHELL COMMANDS ============

void cmd_help() {
    std::cout << CYN "\n  Hydrapulse Commands:\n" RST;
    std::cout << GRN "    help" RST "          — show this help\n";
    std::cout << GRN "    hi" RST "            — greeting\n";
    std::cout << GRN "    clear" RST "         — clear screen\n";
    std::cout << GRN "    uname" RST "         — system info\n";
    std::cout << GRN "    uptime" RST "        — system uptime\n";
    std::cout << GRN "    lsblk" RST "         — list disks\n";
    std::cout << GRN "    hydrainstall" RST "  — install Hydrapulse to disk\n";
    std::cout << GRN "    reboot" RST "        — reboot system\n";
    std::cout << GRN "    exit" RST "          — shutdown\n\n";
}

void cmd_uname() {
    std::string kernel = run_get("uname -r 2>/dev/null");
    std::string arch   = run_get("uname -m 2>/dev/null");
    if (kernel.empty()) kernel = "unknown";
    if (arch.empty())   arch   = "unknown";
    std::cout << "Hydrapulse OS v0.2 (" << arch << ") kernel " << kernel << "\n";
}

void cmd_uptime() {
    std::ifstream f("/proc/uptime");
    double secs = 0;
    if (f >> secs) {
        int h = (int)secs / 3600;
        int m = ((int)secs % 3600) / 60;
        int s = (int)secs % 60;
        std::cout << "up " << h << "h " << m << "m " << s << "s\n";
    } else {
        std::cout << "uptime unavailable\n";
    }
}

void cmd_lsblk() {
    auto disks = detect_disks();
    if (disks.empty()) { std::cout << "no disks found\n"; return; }
    std::cout << CYN "  NAME         SIZE\n" RST;
    for (auto& d : disks)
        std::cout << "  " << d.name << std::string(13 - d.name.size(), ' ')
                  << d.size_h << "\n";
}

// ============ MAIN ============

int main() {
    setsid();
    signal(SIGINT,  SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    init_system();
    print_splash();

    std::string line;
    while (true) {
        std::cout << CYN "hydrapulse" RST "> " << std::flush;
        if (!std::getline(std::cin, line)) break;

        if      (line == "help")          cmd_help();
        else if (line == "hi")            std::cout << "hi i am beta test hydrapulse os\n";
        else if (line == "clear")         std::cout << "\033[2J\033[H";
        else if (line == "uname")         cmd_uname();
        else if (line == "uptime")        cmd_uptime();
        else if (line == "lsblk")         cmd_lsblk();
        else if (line == "hydrainstall")  hydrainstall();
        else if (line == "reboot")      { sync(); reboot(RB_AUTOBOOT); }
        else if (line == "exit" || line == "quit") break;
        else if (!line.empty())
            std::cout << RED "unknown command: " RST << line << "\n";
    }
    return 0;
}
