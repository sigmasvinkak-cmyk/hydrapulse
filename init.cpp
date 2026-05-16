// -------------------------------------------------
// init.cpp – минимальный init (PID 1)
// -------------------------------------------------
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/reboot.h>
#include <signal.h>

void print_prompt() {
    std::cout << "hydrapulse> " << std::flush;
}

int main() {
    // Превратить процесс в лидера сессии (как делает обычный init)
    setsid();
    // Игнорируем сигналы, которые init обычно игнорирует
    signal(SIGINT,  SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    std::string line;
    while (true) {
        print_prompt();
        if (!std::getline(std::cin, line))
            break;               // EOF → выйти

        if (line == "reboot") {
            sync();                         // Синхронно записать диск
            reboot(RB_AUTOBOOT);            // Системный вызов перезагрузки
        } else if (line == "hi") {
            std::cout << "hi i am beta test hydrapulse os" << '\n';
        } else if (line == "exit" || line == "quit") {
            break;
        } else if (!line.empty()) {
            std::cout << "unknown command: " << line << '\n';
        }
    }
    return 0;
}
