#include "network/Client_Handler.h"
#include "network/Protocol_Handler.h"
#include "network/Socket_Server.h"
#include "utils/utils.h"
#include "config.h"

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QThread>

using namespace SocketNetwork;

Client_Handler::Client_Handler(QTcpSocket* socket, const Client_Info& info,
    std::shared_ptr<Database::Database_Manager> db_manager,
    Protocol_Handler* protocol_handler, Socket_Server* server)
    : QObject(nullptr), client_socket(socket), client_info(info), db_manager(db_manager),
      protocol_handler(protocol_handler), server(server), handler_thread(nullptr),
      is_running(false), keep_alive_timer(nullptr)
{
    if (client_socket) {
        client_socket->setParent(this);
        
        // Connect socket signals
        connect(client_socket, &QTcpSocket::readyRead, this, &Client_Handler::handle_ready_read);
        connect(client_socket, &QTcpSocket::disconnected, this, &Client_Handler::handle_disconnection);
    }
    
    // Initialize keep-alive timer
    keep_alive_timer = new QTimer(this);
    connect(keep_alive_timer, &QTimer::timeout, this, [this]() {
        if (get_idle_time() > Config::Server::SOCKET_TIMEOUT_MS) {
            Utils::Logger::warning("Client idle timeout: " + client_info.ip_address);
            handle_disconnection();
        }
    });
}

Client_Handler::~Client_Handler()
{
    stop_handling();
}

void Client_Handler::start_handling()
{
    QMutexLocker locker(&send_mutex);
    if (is_running) {
        return;
    }
    
    is_running = true;
    update_last_activity();
    
    // Start keep-alive timer
    if (keep_alive_timer) {
        keep_alive_timer->start(30000); // Check every 30 seconds
    }
    
    Utils::Logger::info("Client handler started for: " + client_info.ip_address);
}

void Client_Handler::stop_handling()
{
    QMutexLocker locker(&send_mutex);
    if (!is_running) {
        return;
    }
    
    is_running = false;
    
    // Stop keep-alive timer
    if (keep_alive_timer) {
        keep_alive_timer->stop();
    }
    
    // Close socket
    if (client_socket && client_socket->state() != QAbstractSocket::UnconnectedState) {
        client_socket->disconnectFromHost();
        if (client_socket->state() != QAbstractSocket::UnconnectedState) {
            client_socket->waitForDisconnected(3000); // Wait up to 3 seconds
        }
    }
    
    Utils::Logger::info("Client handler stopped for: " + client_info.ip_address);
}

bool Client_Handler::is_client_running() const
{
    return is_running && is_socket_valid();
}

bool Client_Handler::send_message(const QString& message)
{
    QMutexLocker locker(&send_mutex);
    
    if (!is_socket_valid()) {
        return false;
    }
    
    try {
        QString full_message = message + "\r\n";
        QByteArray data = full_message.toUtf8();
        
        qint64 bytes_written = client_socket->write(data);
        if (bytes_written == -1) {
            return false;
        }
        
        if (!client_socket->waitForBytesWritten(5000)) { // 5 second timeout
            return false;
        }
        
        messages_sent++;
        update_last_activity();
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

QString Client_Handler::receive_message()
{
    if (!is_socket_valid()) {
        return QString();
    }
    
    // Check if data is available
    if (!client_socket->canReadLine()) {
        // Wait for data with timeout
        if (!client_socket->waitForReadyRead(Config::Server::SOCKET_TIMEOUT_MS)) {
            return QString();
        }
    }
    
    QByteArray data = client_socket->readLine();
    if (data.isEmpty()) {
        return QString();
    }
    
    QString message = QString::fromUtf8(data).trimmed();
    
    messages_received++;
    update_last_activity();
    
    return message;
}

const Client_Info& Client_Handler::get_client_info() const
{
    return client_info;
}

void Client_Handler::update_last_activity()
{
    client_info.last_activity_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

bool Client_Handler::is_authenticated() const
{
    return client_info.is_authenticated;
}

void Client_Handler::set_authenticated(int user_id, const QString& username)
{
    client_info.is_authenticated = true;
    client_info.user_id = user_id;
    client_info.username = username;
    update_last_activity();
}

qint64 Client_Handler::get_idle_time() const
{
    QDateTime last_activity = QDateTime::fromString(client_info.last_activity_time, "yyyy-MM-dd hh:mm:ss");
    QDateTime now = QDateTime::currentDateTime();
    return last_activity.msecsTo(now);
}

void Client_Handler::handle_ready_read()
{
    if (!is_running) {
        return;
    }
    
    while (client_socket && client_socket->canReadLine()) {
        QString message = receive_message();
        if (!message.isEmpty()) {
            emit messageReceived(message);
            
            // Process message directly
            if (!process_message(message)) {
                handle_disconnection();
                return;
            }
        }
    }
}

void Client_Handler::handle_disconnection()
{
    if (is_running) {
        is_running = false;
        
        Utils::Logger::info("Client disconnected: " + client_info.ip_address);
        
        emit clientDisconnected();
        
        // Notify server to remove this client
        if (server) {
            server->remove_client(client_socket);
        }
    }
}

void Client_Handler::handle_client_loop()
{
    // This method is no longer needed since we use Qt's signal-slot mechanism
    // The functionality is handled by handle_ready_read() slot
}

bool Client_Handler::process_message(const QString& message)
{
    if (!protocol_handler) {
        send_error_response(Config::ErrorMessages::SERVER_ERROR);
        return false;
    }
    
    try {
        auto parsed_message = protocol_handler->parse_message(message);
        
        if (!parsed_message.is_valid) {
            send_error_response(parsed_message.error_message);
            return true; // Continue handling other messages
        }
        
        auto response = protocol_handler->process_message(parsed_message, this);
        
        QString response_str;
        if (response.success) {
            response_str = Utils::JSON::create_success_response(response.data, response.message);
        } else {
            response_str = Utils::JSON::create_error_response(response.message, response.error_code);
        }
        
        return send_message(response_str);
    }
    catch (const std::exception& e) {
        send_error_response("Message processing error: " + QString::fromStdString(e.what()));
        return true;
    }
}

bool Client_Handler::is_socket_valid() const
{
    return client_socket && 
           client_socket->state() == QAbstractSocket::ConnectedState && 
           is_running;
}

void Client_Handler::send_error_response(const QString& error_message)
{
    QString response = Utils::JSON::create_error_response(error_message);
    send_message(response);
}

void Client_Handler::send_success_response(const QString& data, const QString& message)
{
    QString response = Utils::JSON::create_success_response(data, message);
    send_message(response);
}

