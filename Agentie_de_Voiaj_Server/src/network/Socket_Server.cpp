#include "network/Socket_Server.h"
#include "network/Client_Handler.h"
#include "network/Protocol_Handler.h"
#include "utils/utils.h"
#include "config.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>

using namespace SocketNetwork;

Socket_Server::Socket_Server(QObject* parent)
    : QObject(parent), tcp_server(nullptr), is_running(false), is_initialized(false),
      accept_thread(nullptr), cleanup_timer(nullptr), client_count(0),
      total_connections(0), total_messages_received(0), total_messages_sent(0)
{
    server_start_time = Utils::DateTime::get_current_date_time();
    Utils::Logger::info("Socket_Server created with default configuration");
    
    // Set default configuration
    config.ip_address = "127.0.0.1";
    config.port = Config::Server::PORT;
    config.max_clients = Config::Server::MAX_CONNECTIONS;
    config.enable_logging = Config::Application::DEBUG_MODE;
}

Socket_Server::Socket_Server(const Server_Config& config, QObject* parent)
    : QObject(parent), tcp_server(nullptr), config(config), is_running(false), is_initialized(false),
      accept_thread(nullptr), cleanup_timer(nullptr), client_count(0),
      total_connections(0), total_messages_received(0), total_messages_sent(0)
{
    server_start_time = Utils::DateTime::get_current_date_time();
    Utils::Logger::info("Socket_Server created with custom configuration - Port: " + 
                        QString::number(config.port));
}

Socket_Server::~Socket_Server()
{
    stop();
    Utils::Logger::info("Socket_Server destroyed");
}

bool Socket_Server::initialize()
{
    QMutexLocker locker(&clients_mutex);
    if (is_initialized) 
    {
        Utils::Logger::warning("Socket_Server already initialized");
        return true;
    }

    try {
        // Create TCP server
        tcp_server = new QTcpServer(this);
        
        // Connect server signals
        connect(tcp_server, &QTcpServer::newConnection, this, &Socket_Server::handle_new_connection);
        
        // Create protocol handler
        if (db_manager) {
            protocol_handler = std::make_unique<Protocol_Handler>(db_manager);
        }
        
        // Create cleanup timer
        cleanup_timer = new QTimer(this);
        connect(cleanup_timer, &QTimer::timeout, this, &Socket_Server::cleanup_inactive_clients);
        
        is_initialized = true;
        Utils::Logger::info("Socket_Server initialized successfully on " + 
                           config.ip_address + ":" + QString::number(config.port));
        return true;
    }
    catch (const std::exception& e) {
        Utils::Logger::error("Exception during initialization: " + QString::fromStdString(e.what()));
        return false;
    }
}

bool Socket_Server::start()
{
    QMutexLocker locker(&clients_mutex);
    if (is_running) {
        Utils::Logger::warning("Socket_Server is already running");
        return true;
    }

    if (!is_initialized) {
        Utils::Logger::error("Socket_Server not initialized. Call initialize() first.");
        return false;
    }

    if (!tcp_server) {
        Utils::Logger::error("TCP server not created");
        return false;
    }

    try {
        // Parse IP address
        QHostAddress host_address;
        if (config.ip_address == "0.0.0.0") {
            host_address = QHostAddress::Any;
        } else if (config.ip_address == "127.0.0.1" || config.ip_address == "localhost") {
            host_address = QHostAddress::LocalHost;
        } else {
            host_address = QHostAddress(config.ip_address);
        }

        // Start listening
        if (!tcp_server->listen(host_address, config.port)) {
            Utils::Logger::error("Failed to start server: " + tcp_server->errorString());
            return false;
        }

        is_running = true;
        
        // Start cleanup timer
        if (cleanup_timer) {
            cleanup_timer->start(30000); // Cleanup every 30 seconds
        }

        Utils::Logger::info("Server listening on " + config.ip_address + ":" + QString::number(config.port));
        Utils::Logger::info("Maximum connections: " + QString::number(config.max_clients));
        
        return true;
    }
    catch (const std::exception& e) {
        Utils::Logger::error("Exception during server start: " + QString::fromStdString(e.what()));
        return false;
    }
}

void Socket_Server::stop()
{
    QMutexLocker locker(&clients_mutex);
    if (!is_running) {
        return;
    }

    Utils::Logger::info("Stopping Socket_Server...");
    
    is_running = false;

    // Stop cleanup timer
    if (cleanup_timer) {
        cleanup_timer->stop();
    }

    // Close server
    if (tcp_server) {
        tcp_server->close();
    }

    // Stop all client handlers
    for (auto it = active_clients.begin(); it != active_clients.end(); ++it) {
        if (it.value()) {
            it.value()->stop_handling();
        }
    }
    active_clients.clear();
    client_count = 0;

    Utils::Logger::info("Socket_Server stopped successfully");
}

void Socket_Server::set_database_manager(std::shared_ptr<Database::Database_Manager> db_manager)
{
    QMutexLocker locker(&protocol_handler_mutex);
    this->db_manager = db_manager;
    
    // Recreate protocol handler with new database manager
    if (db_manager) {
        protocol_handler = std::make_unique<Protocol_Handler>(db_manager);
        Utils::Logger::info("Database manager set for Socket_Server");
    }
}

void Socket_Server::handle_new_connection()
{
    if (!is_running) {
        return;
    }

    while (tcp_server && tcp_server->hasPendingConnections()) {
        QTcpSocket* client_socket = tcp_server->nextPendingConnection();
        
        if (!client_socket) {
            continue;
        }

        // Check connection limit
        if (client_count >= config.max_clients) {
            Utils::Logger::warning("Connection limit reached. Rejecting client: " + 
                                 client_socket->peerAddress().toString());
            client_socket->disconnectFromHost();
            client_socket->deleteLater();
            continue;
        }

        // Create client info
        Client_Info client_info(client_socket, 
                               client_socket->peerAddress().toString(),
                               client_socket->peerPort());

        Utils::Logger::info("New client connected: " + client_info.ip_address + ":" + 
                           QString::number(client_info.port));

        // Create client handler
        QMutexLocker locker(&protocol_handler_mutex);
        auto client_handler = std::make_shared<Client_Handler>(
            client_socket, client_info, db_manager, protocol_handler.get(), this);

        // Add to active clients
        {
            QMutexLocker clients_locker(&clients_mutex);
            active_clients[client_socket] = client_handler;
            client_count++;
            total_connections++;
        }

        // Connect client handler signals
        connect(client_handler.get(), &Client_Handler::clientDisconnected, 
                this, &Socket_Server::handle_client_disconnected);
        connect(client_handler.get(), &Client_Handler::messageReceived,
                this, [this](const QString& message) {
                    total_messages_received++;
                });

        // Start handling client
        client_handler->start_handling();

        Utils::Logger::info("Client handler created and started for: " + client_info.ip_address);
    }
}

void Socket_Server::handle_client_disconnected()
{
    Client_Handler* client_handler = qobject_cast<Client_Handler*>(sender());
    if (!client_handler) {
        return;
    }

    // Find and remove client
    QMutexLocker locker(&clients_mutex);
    for (auto it = active_clients.begin(); it != active_clients.end(); ++it) {
        if (it.value().get() == client_handler) {
            Utils::Logger::info("Client disconnected: " + it.value()->get_client_info().ip_address);
            active_clients.erase(it);
            client_count--;
            break;
        }
    }
}

void Socket_Server::remove_client(QTcpSocket* client_socket)
{
    if (!client_socket) {
        return;
    }

    QMutexLocker locker(&clients_mutex);
    auto it = active_clients.find(client_socket);
    if (it != active_clients.end()) {
        Utils::Logger::info("Removing client: " + it.value()->get_client_info().ip_address);
        
        // Stop the client handler
        it.value()->stop_handling();
        
        // Remove from active clients
        active_clients.erase(it);
        client_count--;
    }
}

void Socket_Server::cleanup_inactive_clients()
{
    if (!is_running) {
        return;
    }

    QMutexLocker locker(&clients_mutex);
    
    for (auto it = active_clients.begin(); it != active_clients.end();) {
        if (!it.value() || !it.value()->is_client_running()) {
            Utils::Logger::info("Cleaning up inactive client");
            it = active_clients.erase(it);
            client_count--;
        } else {
            // Check for idle timeout
            if (it.value()->get_idle_time() > Config::Server::SOCKET_TIMEOUT_MS) {
                Utils::Logger::info("Client idle timeout: " + it.value()->get_client_info().ip_address);
                it.value()->stop_handling();
                it = active_clients.erase(it);
                client_count--;
            } else {
                ++it;
            }
        }
    }
}

void Socket_Server::send_message_to_client(QTcpSocket* client_socket, const QString& message)
{
    if (!client_socket) {
        return;
    }

    QMutexLocker locker(&clients_mutex);
    auto it = active_clients.find(client_socket);
    if (it != active_clients.end() && it.value()) {
        if (it.value()->send_message(message)) {
            total_messages_sent++;
        }
    }
}

Server_Stats Socket_Server::get_server_stats() const
{
    Server_Stats stats;
    stats.active_clients = client_count;
    stats.total_connections = total_connections;
    stats.total_messages_received = total_messages_received;
    stats.total_messages_sent = total_messages_sent;
    stats.start_time = server_start_time;
    
    // Calculate uptime
    QDateTime start = QDateTime::fromString(server_start_time, "yyyy-MM-dd hh:mm:ss");
    QDateTime now = QDateTime::currentDateTime();
    qint64 uptime_ms = start.msecsTo(now);
    
    int uptime_seconds = uptime_ms / 1000;
    int hours = uptime_seconds / 3600;
    int minutes = (uptime_seconds % 3600) / 60;
    int seconds = uptime_seconds % 60;
    
    stats.uptime = QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
    
    // Calculate average response time (simplified)
    stats.average_response_time_ms = 50.0; // Mock value
    
    // Get memory usage
    stats.memory_usage_mb = static_cast<int>(Utils::Memory::get_memory_usage_MB());
    
    return stats;
}

void Socket_Server::reset_server_stats()
{
    total_connections = 0;
    total_messages_received = 0;
    total_messages_sent = 0;
    server_start_time = Utils::DateTime::get_current_date_time();
    
    Utils::Logger::info("Server statistics reset");
}

bool Socket_Server::is_server_running() const
{
    return is_running && tcp_server && tcp_server->isListening();
}

int Socket_Server::get_active_client_count() const
{
    return client_count;
}

QString Socket_Server::get_server_address() const
{
    if (tcp_server && tcp_server->isListening()) {
        return tcp_server->serverAddress().toString() + ":" + QString::number(tcp_server->serverPort());
    }
    return config.ip_address + ":" + QString::number(config.port);
}

