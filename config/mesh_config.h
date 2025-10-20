// Mesh Network Configuration
#ifndef MESH_CONFIG_H
#define MESH_CONFIG_H

#define MESH_PREFIX     "SIM_MESH"
#define MESH_PASSWORD   "sim_mesh_secure_2024"
#define MESH_PORT       5555

#define MAX_NETWORK_HOPS 6
#define HEARTBEAT_INTERVAL 10000  // 10 seconds
#define NODE_JOIN_TIMEOUT 45000   // 45 seconds

#define MESH_ENCRYPTION AES256

#endif // MESH_CONFIG_H