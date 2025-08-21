#pragma once
#include <QString>

namespace Config
{
	// Server Configuration
	namespace Server
	{
		constexpr int PORT = 8080;
		constexpr int MAX_CONNECTIONS = 100;
		constexpr int BACKLOG_SIZE = 10;
		constexpr int BUFFER_SIZE = 4096;
		constexpr int SOCKET_TIMEOUT_MS = 30000; // 30 seconds
		constexpr bool ENABLE_KEEP_ALIVE = true; // Enable TCP keep-alive
	}

	// Database Configuration
	namespace Database
	{
		const QString DEFAULT_SERVER = "localhost";
		const QString DEFAULT_DATABASE = "Agentie_de_Voiaj";
		const QString DRIVER = "QODBC";
		constexpr int CONNECTION_TIMEOUT = 30; // seconds
		constexpr int QUERY_TIMEOUT = 15; // seconds
		constexpr bool AUTO_COMMIT = true; // Auto-commit transactions

		// Connection string template - not used, build_connection_string() used instead
		const QString CONNECTION_TEMPLATE =
			"DRIVER={" + DRIVER + "};"
			"SERVER=" + DEFAULT_SERVER + ";"
			"DATABASE=" + DEFAULT_DATABASE + ";"
			"Trusted_Connection=yes;"
			"Connection Timeout=" + QString::number(CONNECTION_TIMEOUT) + ";";
	}

	// Application Configuration
	namespace Application
	{
		const QString APP_NAME = "Agentie de Voiaj Server";
		const QString VERSION = "1.0.0";
		constexpr bool DEBUG_MODE = true; // Enable debug mode for logging
		constexpr bool LOG_SQL_QUERIES = false; // Log SQL queries (disabled for performance)
		constexpr bool LOG_CLIENT_REQUESTS = true; // Log client requests

		// File paths
		const QString LOG_DIRECTORY = "logs/";
		const QString CONFIG_DIRECTORY = "config/";
		const QString SQL_SCRIPTS_DIRECTORY = "sql/";
	}

	// JSON Message Configuration
	namespace JSON
	{
		constexpr int MAX_JSON_SIZE = 1024 * 1024; // 1 MB
		constexpr bool PRETTY_PRINT = true; // Enable pretty printing for JSON responses
		constexpr bool VALIDATE_SCHEMA = true; // Validate JSON against schema
	}

	// Security Configuration
	namespace Security
	{
		constexpr int MIN_PASSWORD_LENGTH = 6;
		constexpr int MAX_LOGIN_ATTEMPTS = 5;
		constexpr int LOCKOUT_DURATION_MINUTES = 15;
		constexpr bool REQUIRE_EMAIL_VALIDATION = false;
		// Salt is now per-user (username) for better security
	}

	// Business Logic Configuration
	namespace Business
	{
		constexpr int MAX_PERSONS_PER_RESERVATION = 10;
		constexpr qreal MIN_OFFER_PRICE = 1.0;
		constexpr qreal MAX_OFFER_PRICE = 50000.0;
		constexpr int MAX_RESERVATION_DURATION_DAYS = 365;
		constexpr int MIN_ADVANCE_BOOKING_DAYS = 1;
	}

	// Error Messages
	namespace ErrorMessages
	{
		const QString DB_CONNECTION_FAILED = "Failed to connect to database";
		const QString DB_QUERY_FAILED = "Database query failed";
		const QString INVALID_JSON = "Invalid JSON format";
		const QString AUTHENTICATION_FAILED = "Authentication failed";
		const QString USER_NOT_FOUND = "User not found";
		const QString OFFER_NOT_FOUND = "Offer not found";
		const QString INSUFFICIENT_SEATS = "Not enough available seats";
		const QString INVALID_REQUEST = "Invalid request format";
		const QString SERVER_ERROR = "Internal server error";
		const QString SOCKET_COMM_ERROR = "Socket communication error";
	}

	// Success Messages
	namespace SuccessMessages
	{
		const QString USER_CREATED = "User created successfully";
		const QString LOGIN_SUCCESS = "Login successful";
		const QString RESERVATION_CREATED = "Reservation created successfully";
		const QString RESERVATION_CANCELLED = "Reservation cancelled successfully";
		const QString DATA_RETRIEVED = "Data retrieved successfully";
	}
}