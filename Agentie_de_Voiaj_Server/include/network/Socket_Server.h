#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <memory>
#include <functional>

#include "network/Network_Types.h"
#include "database/Database_Manager.h"

// Forward declarations
class Protocol_Handler;

namespace SocketNetwork
{
	class Client_Handler;
}

namespace SocketNetwork
{
	class Socket_Server : public QObject
	{
		Q_OBJECT

	private:
		QTcpServer* tcp_server;
		Server_Config config;
		std::shared_ptr<Database::Database_Manager> db_manager;
		std::unique_ptr<Protocol_Handler> protocol_handler;

		bool is_running;
		bool is_initialized;
		QThread* accept_thread;
		QTimer* cleanup_timer;

		QHash<QTcpSocket*, std::shared_ptr<Client_Handler>> active_clients;
		QMutex clients_mutex;
		QMutex protocol_handler_mutex;
		int client_count;

		int total_connections;
		int total_messages_received;
		int total_messages_sent;
		QString server_start_time;

	public:
		explicit Socket_Server(QObject* parent = nullptr);
		explicit Socket_Server(const Server_Config& config, QObject* parent = nullptr);
		~Socket_Server();

		bool initialize();
		bool start();
		void stop();
		void set_database_manager(std::shared_ptr<Database::Database_Manager> db_manager);

		bool is_server_running() const;
		int get_active_client_count() const;
		QString get_server_address() const;
		void send_message_to_client(QTcpSocket* client_socket, const QString& message);

		Server_Stats get_server_stats() const;
		void reset_server_stats();

	signals:
		void clientConnected(const Client_Info& client);
		void clientDisconnected(const Client_Info& client);
		void messageReceived(const Client_Info& client, const QString& message);
		void serverError(const QString& error);

	// Friend declaration to allow Client_Handler to access private members
	friend class Client_Handler;

	private slots:
		void handle_new_connection();
		void handle_client_disconnected();
		void cleanup_inactive_clients();

	public:
		void remove_client(QTcpSocket* client_socket);

	private:
		// Additional private methods can be added here if needed
	};
}