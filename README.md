# Backup Client and Server

## Overview

1. The server opens a TCP socket and listens to incoming connections from any IP addresses on a given port.

2. The client reads its configuration file (config.txt), parses, and verifies options for server IP, port, etc.

3. The client opens a TCP socket and connects to the server IP address and port specified in the configuration file.

4. The client recursively searches through the synchronization path for files and calculates their checksums, which is then sent to the server.

5. The server receives paths for files and their checksums from the client, which it then compares to its own checksums stored in a file, and sends paths with mismatched checksums or not in the checksum file, which are outdated, over to the client.

6. The client gets paths for files that are not up to date on the server, and sends those files to the server.

7. The server receives outdated files from the client and updates or creates them and their necessary directories.

8. The files are now up to date on the server.
