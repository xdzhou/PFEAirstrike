#include "networkmanager.h"
#include "messages.h"
#include "keys.h"
#include <QKeyEvent>
#include <QTimer>

NetworkManager::NetworkManager(QString ip_addr, int port)
{
    this->ip_addr=ip_addr;
    this->port = port;
    next_time = 0;
    keep_running = 42;
    myClientId = -1;
}

NetworkManager::~NetworkManager()
{

}

int NetworkManager::network_init(){
    writeText("Initializing ENet.");

    if (enet_initialize() != 0) {
        emit writeText("An error occurred while initializing ENet");
        return -1;
    }
    emit writeText("ENet started.");
    client = enet_host_create(NULL /* create a client host */ ,
                              1 /* only allow 1 outgoing connection */ ,
                              2 /* allow up 2 channels to be used, 0 and 1 */ ,
                              57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */ ,
                              14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */ );

    if (client == NULL) {
        writeText("An error occurred while trying to create an ENet client host.");
        return -1;
    }

    /* Connect to some.server.net:1234. */
    // enet_address_set_host(&address, ip_addr.toStdString().c_str());
    //address.port = port;

    enet_address_set_host(&address, ip_addr.toStdString().c_str());
    address.port = port;

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    int trying_left = 5;
    int connect_succeeded=0;
    while (!connect_succeeded){
        writeText("Trying to connect to " + ip_addr + ":" + QString::number(port) + "...");
        /* Initiate the connection, allocating the two channels 0 and 1. */
        peer = enet_host_connect(client, &address, 2, 0);

        if (peer == NULL) {
            writeText("No available peers for initiating an ENet connection.");
            return -1;
        }

        if (enet_host_service(client, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
            writeText("Connection to " + ip_addr + ":" + QString::number(port) + "succeeded.");
            connect_succeeded=1;
            sendMessage(MSG_HELLO,myClientId);
        } else {
            /* Either the 5 seconds are up or a disconnect event was */
            /* received. Reset the peer in the event the 5 seconds   */
            /* had run out without any significant event.            */
            enet_peer_reset(peer);
            //printf("Connection to %s:%d failed.\n",ip_addr,port);
            trying_left--;
            if(trying_left<=0){
                writeText("Could not connect to " + ip_addr + ":" + port);
                return -1;
            }
        }
    }
    start();
}

void NetworkManager::process_packet(ENetEvent * event){
    AS_message_t * msg = (AS_message_t * )(event->packet->data);
    int peerID = event->peer->incomingPeerID;
    //writeText("Message : ");
    switch (msg->mess_type) {
    case MSG_POINTS:
        //writeText("My Score is : " + QString::number(msg->data));
        break;
    case MSG_HELLO:
        myClientId = msg->client_id;
        writeText("Hello received");
        break;
    case MSG_NO_SPACE:
        writeText("Plus de place, reconnectez-vous plus tard.");
        break;
    default:
        break;
    }
    enet_packet_destroy(event->packet);
}

void NetworkManager::update_state(){
    int current_time = time(NULL);
    if (current_time>next_time){
        set_key(KEY__ACCELERATE);
        next_time = current_time+(rand()%1)+1;
        set_rand_key(KEY_FIRE);
        //printf("%d",current_time);
    }
}

void NetworkManager::set_rand_key(int key){
    AS_message_t msg;
    msg.client_id = 0;
    msg.mess_type=MSG_KEY;

    if (rand()>(RAND_MAX/2)){
        msg.data=key;
    }else{
        msg.data=-key;
    }
    ENetPacket *packet = enet_packet_create(&msg, sizeof(AS_message_t), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

void NetworkManager::set_key(int key){
    sendMessage(MSG_KEY, myClientId, key);
    /*
    AS_message_t msg;
    msg.client_id = 0;
    msg.mess_type=MSG_KEY;
    msg.data=key;
    //ENetPacket *packet = enet_packet_create(&msg, sizeof(AS_message_t), ENET_PACKET_FLAG_RELIABLE);
    ENetPacket *packet = enet_packet_create(&msg, sizeof(msg), ENET_PACKET_FLAG_RELIABLE);
    int rc = enet_peer_send(peer, 0, packet);
   // writeText("Key = " + QString::number(key));
   */
}

void NetworkManager::network_loop(){
    int serviceResult;
    /* Keep doing host_service until no events are left */
    while ( (serviceResult=enet_host_service(client, &event, 0))!=0) {
        if (serviceResult > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                //printf("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
                event.peer->data = (void *)"New User";
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                process_packet(&event);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                writeText("Server disconected.\n");
                exit(EXIT_FAILURE);

                break;
            case ENET_EVENT_TYPE_NONE:
                break;
            }
        } else if (serviceResult < 0) {
            writeText("Error with servicing the client");
            exit(EXIT_FAILURE);
        }
    }
}

void NetworkManager::testFunction()
{
    emit writeText("Test");
}

void NetworkManager::sendMessage(int msgType, int clientId, int data)
{
   // ENetPeer *p = &server->peers[peerId];
    if (!(peer==NULL)){
        AS_message_t msg;
        msg.mess_type=msgType;
        msg.client_id = clientId;
        msg.data = data;
        msg.name[0] = '\0';
        ENetPacket *packet = enet_packet_create(&msg, sizeof(AS_message_t), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
    }
}

void NetworkManager::process_key(QKeyEvent * event, int key_status){
    //writeText("Process key");
    if (event->key() == Qt::Key_Right )
        set_key(key_status*KEY__DOWN);
    if (event->key() == Qt::Key_Left)
        set_key(key_status*KEY__UP);
    if (event->key() == Qt::Key_Up)
        set_key(key_status*KEY__ACCELERATE);
    if (event->key() == Qt::Key_Space)
        set_key(key_status*KEY_FIRE);
    if (event->key() == Qt::Key_Control)
        set_key(key_status*KEY_BOMB);
}

void NetworkManager::start()
{
    network_loop();
    QTimer::singleShot(20, this, SLOT(start()));
}