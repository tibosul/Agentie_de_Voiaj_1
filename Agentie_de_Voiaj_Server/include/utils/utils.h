#pragma once
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStorageInfo>
#include <QtCore/QCryptographicHash>
#include <QtCore/QRandomGenerator>
#include <QtCore/QUuid>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QAbstractSocket>
#include <exception>

namespace Utils
{
	// String Utilities
	namespace String
	{
		QString trim(const QString& str);
		QString to_lower(const QString& str);
		QString to_upper(const QString& str);
		QStringList split(const QString& str, QChar delimiter);
		bool starts_with(const QString& str, const QString& prefix);
		bool ends_with(const QString& str, const QString& suffix);
		QString replace(const QString& str, const QString& from, const QString& to);
		bool is_empty(const QString& str);
		QString escape_SQL(const QString& input);
	}

	// Date and Time Utilities
	namespace DateTime
	{
		QString get_current_date_time();
		QString get_current_date();
		QString get_current_time();
		QString format_date_time(const QDateTime& dt);
		bool is_valid_date(const QString& date_str); // format: YYYY-MM-DD
		bool is_date_in_future(const QString& date_str); // format: YYYY-MM-DD
		int days_between(const QString& date1, const QString& date2); // format: YYYY-MM-DD
		QString add_days_to_date(const QString& date_str, int days); // format: YYYY-MM-DD
	}

	// Validation Utilities
	namespace Validation
	{
		bool is_valid_username(const QString& username);
		bool is_valid_password(const QString& password);
		bool is_valid_email(const QString& email);
		bool is_valid_phone_number(const QString& phone);
		bool is_valid_cnp(const QString& cnp);
		bool is_positive_number(qreal value);
		bool is_valid_price_range(qreal min_price, qreal max_price);
		bool is_valid_person_count(int count);
	}

	// Conversion Utilities
	namespace Conversion
	{
		QString double_to_string(qreal value, int precision = 2);
		qreal string_to_double(const QString& str);
		int string_to_int(const QString& str);
		QString int_to_string(int value);
		QString bool_to_string(bool value);
		bool string_to_bool(const QString& str);
	}

	// Cryptography Utilities
	namespace Crypto
	{
		QString hash_password(const QString& password, const QString & salt = "");
		QString generate_salt();
		bool verify_password(const QString& password, const QString& hashed_password, const QString& salt = "");
		QString generate_session_token();
		QString md5_hash(const QString& input);
	}

	// File Utilities
	namespace File
	{
		bool file_exists(const QString& filepath);
		QString read_file(const QString& filepath);
		bool write_file(const QString& filepath, const QString& content);
		bool create_directory(const QString& directory_path);
		QStringList list_files(const QString& directory_path, const QString& extension = "");
		QString get_file_extension(const QString& filepath);
		QString get_file_name(const QString& filepath);
	}

	// Network Utilities
	namespace Network
	{
		QString get_local_ip();
		bool is_valid_ip(const QString& ip);
		bool is_valid_port(int port);
		QString get_socket_error_message(int error_code);
		QString format_client_address(const QString& ip, int port);
	}

	// Logging Utilities
	namespace Logger
	{
		enum class Level
		{
			DEBUG = 0,
			INFO = 1,
			WARNING = 2,
			ERR = 3,
			CRITICAL = 4
		};

		void log(Level level, const QString& message);
		void debug(const QString& message);
		void info(const QString& message);
		void warning(const QString& message);
		void error(const QString& message);
		void critical(const QString& message);
		void log_to_file(const QString& filename, const QString& message);
		void set_log_level(Level min_level);
		QString level_to_string(Level level);
		void initialize_logging();
		void enable_file_logging(bool enabled);
		QString get_log_filename();
	}

	// JSON Utilities
	namespace JSON
	{
		bool is_valid_json(const QString& json_str);
		QString escape_json(const QString& input);
		QString create_error_response(const QString& error_message, int error_code = -1);
		QString create_success_response(const QString& data = QString(), const QString& message = "");
		QString format_json(const QString& json_str);
	}

	// Random Utilities
	namespace Random
	{
		int generate_int(int min, int max);
		qreal generate_double(qreal min, qreal max);
		QString generate_random_string(size_t length, bool include_numbers = true, bool include_symbols = false);
		QString generate_uuid();
	}

	// Memory Utilities
	namespace Memory
	{
		qint64 get_memory_usage_MB();
		qint64 get_available_memory_MB();
		void log_memory_usage(const QString& context = "");
		void log_system_info();
	}

	// Performance Utilities
	namespace Performance
	{
		class Timer
		{
		private:
			QDateTime start_time;
			QElapsedTimer elapsed_timer;
			QString operation_name;

		public:
			Timer(const QString& name);
			~Timer();
			void reset();
			qreal elapsed_milliseconds() const;
		};

		void start_operation(const QString& operation_name);
		void end_operation(const QString& operation_name);
		void log_performance(const QString& operation, qreal duration_ms);
	}

	namespace Exceptions
	{
		class BaseException : public std::exception
		{
		protected:
			QString message_;
			int error_code_;

		public:
			BaseException(const QString& message, int code = -1)
				: message_(message), error_code_(code) {
			}

			const char* what() const noexcept override { return message_.toLocal8Bit().constData(); }
			int error_code() const { return error_code_; }
			const QString& message() const { return message_; }
		};

		class DatabaseException : public BaseException
		{
		public:
			DatabaseException(const QString& message, int code = -1)
				: BaseException("[DATABASE] " + message, code) {
			}
		};

		class NetworkException : public BaseException
		{
		public:
			NetworkException(const QString& message, int code = -1)
				: BaseException("[NETWORK] " + message, code) {
			}
		};

		class ValidationException : public BaseException
		{
		public:
			ValidationException(const QString& message, int code = -1)
				: BaseException("[VALIDATION] " + message, code) {
			}
		};

		class ConfigurationException : public BaseException
		{
		public:
			ConfigurationException(const QString& message, int code = -1)
				: BaseException("[CONFIG] " + message, code) {
			}
		};
	}
}