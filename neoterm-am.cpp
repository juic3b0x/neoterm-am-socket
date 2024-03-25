/**
 *    neoterm-am sends commands to the NeoTerm:API app through a unix socket
 *    Copyright (C) 2021 Tarek Sander
 *
 *    This program is free software: you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation, version 3.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <string>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>

#include "neoterm-am.h"

void send_blocking(const int fd, const char* data, int len) {
    while (len > 0) {
        int ret = send(fd, data, len, MSG_NOSIGNAL);
        if (ret == -1) {
            perror("Socket write error");
            exit(1);
        }
        len -= ret;
        data += ret;
    }
}


bool recv_part(const int fd, char* data, int len) {
    while (len > 0) {
        int ret = recv(fd, data, 1, 0);
        if (ret == -1) {
            perror("Socket read error");
            exit(1);
        }
        if (*data == '\0' || ret == 0) {
            return true;
        }
        data++;
        len--;
    }
    return false;
}

bool is_number(const std::string& s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}


int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "neoterm-am-socket only expects 1 argument and received " << argc - 1 << std::endl;
        return 1;
    }

    struct sockaddr_un adr = {.sun_family = AF_UNIX};
    if (strlen(SOCKET_PATH) >= sizeof(adr.sun_path)) {
        std::cerr << "Socket path \"" << SOCKET_PATH << "\" too long" << std::endl;
        return 1;
    }

    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Could not create socket");
        return 1;
    }

    strncpy(adr.sun_path, SOCKET_PATH, sizeof(adr.sun_path)-1);
    if (connect(fd, (struct sockaddr*) &adr, sizeof(adr)) == -1) {
        perror("Could not connect to socket");
        close(fd);
        return 1;
    }

    if (argc == 2) {
        send_blocking(fd, argv[1], strlen(argv[1]));
    } 

    shutdown(fd, SHUT_WR);

    int exit_code;

    {
        char tmp[10];
        memset(tmp, '\0', sizeof(tmp));
        if (!recv_part(fd, tmp, sizeof(tmp) - 1)) {
            std::string errmsg = "Exit code \"" + std::string(tmp) + "\" is too long. It must be valid number between 0-255";
            if (errno == 0)
                std::cerr << errmsg << std::endl;
            else
                perror(errmsg.c_str());
            close(fd);
            return 1;
        }

        // strtol returns 0 if conversion failed and allows leading whitespaces
        errno = 0;
        exit_code = strtol(tmp, NULL, 10);
        if (!is_number(std::string(tmp)) || (exit_code == 0 && std::string(tmp) != "0")) {
            std::cerr << "Exit code \"" + std::string(tmp) + "\" is not a valid number between 0-255" << std::endl;
            close(fd);
            return 1;
        }

        // Out of bound exit codes would return with exit code `44` `Channel number out of range`.
        if (exit_code < 0 || exit_code > 255) {
            std::cerr << "Exit code \"" << std::string(tmp) << "\" is not a valid exit code between 0-255" << std::endl;
            close(fd);
            return 1;
        }
    }

    {
        char tmp[4096];
        bool ret;
        do {
            memset(tmp, '\0', sizeof(tmp));
            ret = recv_part(fd, tmp, sizeof(tmp)-1);
            fputs(tmp, stdout);
        } while (!ret);
    }

    {
        char tmp[4096];
        bool ret;
        do {
            memset(tmp, '\0', sizeof(tmp));
            ret = recv_part(fd, tmp, sizeof(tmp)-1);
            fputs(tmp, stderr);
        } while (!ret);
    }

    close(fd);

    return exit_code;
}
