#pragma once

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtNetwork/QHostAddress>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QTcpSocket>
#include <functional>

#include "config.h"
#include "utils/utils.h"

namespace SocketNetwork
{
	struct Server_Config
	{
		QString ip_address = "127.0.0.1";
		int port = Config::Server::PORT;
		int max_clients = Config::Server::MAX_CONNECTIONS;
		int receive_timeout_ms = Config::Server::SOCKET_TIMEOUT_MS;
		int send_timeout_ms = Config::Server::SOCKET_TIMEOUT_MS;
		int keep_alive_interval_ms = 60000;
		bool enable_logging = Config::Application::DEBUG_MODE;

		Server_Config() = default;
		Server_Config(const QString& ip, int p)
			: ip_address(ip), port(p) 
		{
		}
	};

	struct Client_Info
	{
		QTcpSocket* socket;
		QString ip_address;
		int port;
		QString connection_time;
		QString last_activity_time;
		bool is_authenticated = false;
		int user_id = 0;
		QString username;

		Client_Info(QTcpSocket* s, const QString& ip, int p)
			: socket(s), ip_address(ip), port(p)
		{
			connection_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
			last_activity_time = connection_time;
		}
	};

	struct Server_Stats
	{
		int active_clients;
		int total_connections;
		int total_messages_received;
		int total_messages_sent;
		QString uptime;
		QString start_time;
		qreal average_response_time_ms;
		int memory_usage_mb;
	};

	enum class Message_Type
	{
		AUTHENTICATION,
		REGISTRATION,
		GET_DESTINATIONS,
		GET_OFFERS,
		SEARCH_OFFERS,
		BOOK_OFFER,
		GET_USER_RESERVATIONS,
		CANCEL_RESERVATION,
		GET_USER_INFO,
		UPDATE_USER_INFO,
		// Admin message types reserved for future implementation
		KEEPALIVE,
		ERR,
		UNKNOWN
	};

	struct Parsed_Message
	{
		Message_Type type;
		QString raw_message;
		QJsonObject json_data; // Store parsed JSON data
		bool is_valid = false;
		QString error_message;

		Parsed_Message() : type(Message_Type::UNKNOWN), is_valid(false)
		{
		}
	};

	struct Response
	{
		bool success = false;
		QString message;
		QString data; // JSON data
		int error_code = 0;

		Response(bool s = false, const QString& msg = "", const QString& d = "")
			: success(s), message(msg), data(d)
		{
		}
		
		Response(bool s, const QString& msg, const QString& d, int code)
			: success(s), message(msg), data(d), error_code(code)
		{
		}
	};

	// SocketRAII is no longer needed with Qt's automatic resource management
	// QTcpSocket handles resource cleanup automatically

	// Forward declarations
	class Socket_Server;
	class Client_Handler;
	class Protocol_Handler;
}