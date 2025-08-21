#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>
#include <memory>

#include "network/Network_Types.h"
#include "database/Database_Manager.h"

// Forward declarations

namespace SocketNetwork
{
	class Socket_Server;
	class Protocol_Handler;
}

namespace SocketNetwork
{
	class Client_Handler : public QObject
	{
		Q_OBJECT

	private:
		QTcpSocket* client_socket;
		Client_Info client_info;
		std::shared_ptr<Database::Database_Manager> db_manager;
		Protocol_Handler* protocol_handler;
		Socket_Server* server;

		QThread* handler_thread;
		bool is_running;
		QMutex send_mutex;

		QTimer* keep_alive_timer;
		int messages_received = 0;
		int messages_sent = 0;

	public:
		Client_Handler(QTcpSocket* socket, const Client_Info& info,
			std::shared_ptr<Database::Database_Manager> db_manager,
			Protocol_Handler* protocol_handler, Socket_Server* server);
		~Client_Handler();

		void start_handling();
		void stop_handling();
		bool is_client_running() const;

		bool send_message(const QString& message);
		QString receive_message();

		const Client_Info& get_client_info() const;
		void update_last_activity();
		bool is_authenticated() const;
		void set_authenticated(int user_id, const QString& username);

		int get_messages_received() const 
		{ 
			return messages_received; 
		}
		
		int get_messages_sent() const 
		{ 
			return messages_sent;
		}
		qint64 get_idle_time() const;

	signals:
		void messageReceived(const QString& message);
		void clientDisconnected();

	private slots:
		void handle_ready_read();
		void handle_disconnection();

	private:
		void handle_client_loop();
		bool process_message(const QString& message);
		bool is_socket_valid() const;
		void send_error_response(const QString& error_message);
		void send_success_response(const QString& data = "", const QString& message = "");
	};
}