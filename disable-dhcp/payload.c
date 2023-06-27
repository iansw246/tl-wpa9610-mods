#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define REMOUNTED_SBIN_PATH "/tmp/original_sbin"
#define HACKED_SBIN_PATH "/tmp/hacked_sbin"

int main()
{
    const char* remountedSbinPath = REMOUNTED_SBIN_PATH;
    const char* hackedSbinPath = HACKED_SBIN_PATH;

    int res;

    if (mkdir(remountedSbinPath, 0700) != 0)
    {
        perror("Creating directory " REMOUNTED_SBIN_PATH);
    }

    // Make bind mount to be able to access underlying sbin
    res = mount("/usr/sbin", remountedSbinPath, NULL, MS_BIND, NULL);
    if (res != 0)
    {
        perror("Mounting /usr/sbin");
        return EXIT_FAILURE;
    }

    // Directory to mount over /usr/sbin, with modified udhcpd
    if (mkdir(hackedSbinPath, 0700) != 0)
    {
        perror("Creating directory" HACKED_SBIN_PATH);
    }

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(remountedSbinPath)) != NULL)
    {
        // Changedir so that finding realpath of symlink is easier
        if (chdir(remountedSbinPath) != 0)
        {
            perror("Changing to " REMOUNTED_SBIN_PATH);
            return EXIT_FAILURE;
        }

        while ((ent = readdir(dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            {
                continue;
            }

            // Make symlink in /tmp/hacked_sbin to every file in /usr/sbin
            // so it is a copy of /usr/sbin

            char linkPath[PATH_MAX];
            if (snprintf(linkPath, sizeof(linkPath), HACKED_SBIN_PATH "/%s", ent->d_name) < 0)
            {
                perror("Create symlink targetPath string");
                return EXIT_FAILURE;
            }

            char linkTarget[PATH_MAX];
            if (realpath(ent->d_name, linkTarget) == NULL)
            {
                perror("Getting real path of symbolic link");
                return EXIT_FAILURE;
            }

            printf("Path: %s, target: %s\n", linkPath, linkTarget);

            if (symlink(linkTarget, linkPath) != 0)
            {
                char errorMessage[1024];
                snprintf(errorMessage, sizeof(errorMessage), "Create symlink target: %s, linkPath: %s", linkTarget,
                         linkPath);
                perror(errorMessage);
            }
        }
        closedir(dir);
    }
    else
    {
        perror("Opening " REMOUNTED_SBIN_PATH);
        return EXIT_FAILURE;
    }

    // Make hacked_sbin have udhcpd that returns immediately and doesn't run the actual
    // udhcpd program
    unlink(HACKED_SBIN_PATH "/udhcpd");

    // I think it would be safer to have O_NOFOLLOW, but it doesn't appear to exist with this version of
    // gcc/Linux/glibc, I'm not sure which
    int newUdhcpdFd = open(HACKED_SBIN_PATH "/udhcpd", O_WRONLY | O_CREAT | O_EXCL, 0777);
    if (newUdhcpdFd == -1)
    {
        perror("Opening" HACKED_SBIN_PATH "/udhcpd for writing");
        return EXIT_FAILURE;
    }

    // Replace udhcpd with script that just creates a pid file, so that LanSettingsd thinks udhcpd started successfully
    // and doesn't try to start udhcpd again, which I guess has no effect but seems wasteful
    char newUdhcpdContents[] = "#!/bin/sh\necho $$ > /tmp/udhcpd.pid\n";

    if (write(newUdhcpdFd, newUdhcpdContents, sizeof(newUdhcpdContents)) == -1)
    {
        perror("Writing newUdhcpd");
        return EXIT_FAILURE;
    }

    close(newUdhcpdFd);

    // Mount hacked sbin over actual /usr/sbin
    if (mount(HACKED_SBIN_PATH, "/usr/sbin", NULL, MS_BIND, NULL) != 0)
    {
        perror("Mounting" HACKED_SBIN_PATH "over /usr/sbin");
        return EXIT_FAILURE;
    }

    printf("Payload success!\n");

    return EXIT_SUCCESS;
}
