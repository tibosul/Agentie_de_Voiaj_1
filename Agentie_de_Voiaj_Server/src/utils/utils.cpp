#include "utils/utils.h"
#include "config.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QCryptographicHash>
#include <QtCore/QUuid>
#include <QtCore/QRandomGenerator>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStorageInfo>
#include <QtCore/QJsonParseError>
#include <QtCore/QSysInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QDebug>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QNetworkAddressEntry>

#include <algorithm>

Q_LOGGING_CATEGORY(serverGeneral, "server.general")
Q_LOGGING_CATEGORY(serverDatabase, "server.database")
Q_LOGGING_CATEGORY(serverNetwork, "server.network")
Q_LOGGING_CATEGORY(serverPerformance, "server.performance")
Q_LOGGING_CATEGORY(serverMemory, "server.memory")

namespace Utils
{
	namespace String
	{
		QString trim(const QString& str)
		{
			return str.trimmed();
		}

		QString to_lower(const QString& str)
		{
			return str.toLower();
		}

		QString to_upper(const QString& str)
		{
			return str.toUpper();
		}

		QStringList split(const QString& str, QChar delimiter)
		{
			return str.split(delimiter, Qt::SkipEmptyParts);
		}

		bool starts_with(const QString& str, const QString& prefix)
		{
			return str.startsWith(prefix);
		}

		bool ends_with(const QString& str, const QString& suffix)
		{
			return str.endsWith(suffix);
		}

		QString replace(const QString& str, const QString& from, const QString& to)
		{
			QString result = str;
			return result.replace(from, to);
		}

		bool is_empty(const QString& str)
		{
			return str.trimmed().isEmpty();
		}

		QString escape_SQL(const QString& input)
		{
			QString escaped = input;
			return escaped.replace("'", "''");
		}
	}

	namespace DateTime
	{
		QString get_current_date_time()
		{
			return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
		}

		QString get_current_date()
		{
			return QDate::currentDate().toString("yyyy-MM-dd");
		}

		QString get_current_time()
		{
			return QTime::currentTime().toString("hh:mm:ss");
		}

		QString format_date_time(const QDateTime& dt)
		{
			return dt.toString("yyyy-MM-dd hh:mm:ss");
		}

		bool is_valid_date(const QString& date_str)
		{
			QDate date = QDate::fromString(date_str, "yyyy-MM-dd");
			return date.isValid();
		}

		bool is_date_in_future(const QString& date_str)
		{
			QDate date = QDate::fromString(date_str, "yyyy-MM-dd");
			return date.isValid() && date > QDate::currentDate();
		}

		int days_between(const QString& date1, const QString& date2)
		{
			QDate d1 = QDate::fromString(date1, "yyyy-MM-dd");
			QDate d2 = QDate::fromString(date2, "yyyy-MM-dd");
			if (!d1.isValid() || !d2.isValid())
				return 0;
			return d1.daysTo(d2);
		}

		QString add_days_to_date(const QString& date_str, int days)
		{
			QDate date = QDate::fromString(date_str, "yyyy-MM-dd");
			if (!date.isValid())
				return "";
			return date.addDays(days).toString("yyyy-MM-dd");
		}
	}

	namespace Validation
	{
		bool is_valid_username(const QString& username)
		{
			return !username.isEmpty() && username.length() <= 50 && std::all_of(username.begin(), username.end(), [](QChar c) {
				return c.isLetterOrNumber() || c == '_' || c == '-';
			});
		}

		bool is_valid_password(const QString& password)
		{
			return password.length() >= 6 && password.length() <= 100;
		}

		bool is_valid_email(const QString& email)
		{
			QRegularExpression email_regex(R"((\w+)(\.\w+)*@(\w+)(\.\w+)+)");
			return email_regex.match(email).hasMatch();
		}

		bool is_valid_phone_number(const QString& phone)
		{
			QRegularExpression phone_regex(R"(\+?[0-9\s\-()]{7,15})");
			return phone_regex.match(phone).hasMatch();
		}

		bool is_valid_cnp(const QString& cnp)
		{
			QRegularExpression cnp_regex(R"(\d{13})");
			return cnp_regex.match(cnp).hasMatch();
		}

		bool is_positive_number(qreal value)
		{
			return value > 0;
		}

		bool is_valid_price_range(qreal min_price, qreal max_price)
		{
			return min_price >= 0 && max_price >= min_price;
		}

		bool is_valid_person_count(int count)
		{
			return count > 0 && count <= 100; // Assuming a maximum of 100 persons
		}
	}

	namespace Conversion
	{
		QString double_to_string(qreal value, int precision)
		{
			return QString::number(value, 'f', precision);
		}

		qreal string_to_double(const QString& str)
		{
			try
			{
				return str.toDouble();
			}
			catch (const std::invalid_argument&)
			{
				return 0.0; // or throw an exception
			}
		}

		int string_to_int(const QString& str)
		{
			try
			{
				return str.toInt();
			}
			catch (const std::invalid_argument&)
			{
				return 0; // or throw an exception
			}
		}

		QString int_to_string(int value)
		{
			return QString::number(value);
		}

		QString bool_to_string(bool value)
		{
			return value ? "true" : "false";
		}

		bool string_to_bool(const QString& str)
		{
			return str == "true" || str == "1";
		}
	}

	namespace Crypto
	{
		QString hash_password(const QString& password, const QString& salt)
		{
			QString salted_password = password + salt;
			QCryptographicHash hash(QCryptographicHash::Sha256);
			hash.addData(salted_password.toUtf8());
			return hash.result().toHex();
		}

		QString generate_salt()
		{
			return Utils::Random::generate_random_string(16, true, false);
		}

		bool verify_password(const QString& password, const QString& hashed_password, const QString& salt)
		{
			return hash_password(password, salt) == hashed_password;
		}

		QString generate_session_token()
		{
			return Utils::Random::generate_random_string(32, true, true);
		}

		QString md5_hash(const QString& input)
		{
			QCryptographicHash hash(QCryptographicHash::Md5);
			hash.addData(input.toUtf8());
			return hash.result().toHex();
		}	
	}

	namespace File
	{
		bool file_exists(const QString& filepath)
		{
			return QFile::exists(filepath);
		}

		QString read_file(const QString& filepath)
		{
			if (!file_exists(filepath))
				return "";
			QFile file(filepath);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				return "";
			QTextStream in(&file);
			return in.readAll();
		}

		bool write_file(const QString& filepath, const QString& content)
		{
			QFile file(filepath);
			if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
				return false;
			QTextStream out(&file);
			out << content;
			file.close();
			return true;
		}

		bool create_directory(const QString& directory_path)
		{
			return QDir().mkpath(directory_path);
		}

		QStringList list_files(const QString& directory_path, const QString& extension)
		{
			QStringList files;
			if (!QDir(directory_path).exists() || !QDir(directory_path).isReadable())
				return files;
			for (const auto& entry : QDir(directory_path).entryInfoList(QDir::Files))
			{
				if (entry.isFile())
				{
					if (extension.isEmpty() || entry.suffix() == extension)
						files.push_back(entry.fileName());
				}
			}
			return files;
		}

		QString get_file_extension(const QString& filepath)
		{
			auto pos = filepath.lastIndexOf('.');
			if (pos == -1)
				return "";
			return filepath.mid(pos + 1);
		}

		QString get_file_name(const QString& filepath)
		{
			auto pos = filepath.lastIndexOf('/');
			if (pos == -1)
				return filepath;
			return filepath.mid(pos + 1);
		}
	}

	namespace Network
	{
		QString get_local_ip()
		{
			QString hostName = QHostInfo::localHostName();
			QHostInfo hostInfo = QHostInfo::fromName(hostName);
			
			foreach (const QHostAddress &address, hostInfo.addresses()) {
				if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
					return address.toString();
				}
			}
			
			// Fallback: try to get first non-loopback IPv4 address from network interfaces
			foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
				if (interface.flags().testFlag(QNetworkInterface::IsUp) && 
					!interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
					foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
						if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
							return entry.ip().toString();
						}
					}
				}
			}
			
			return QString("127.0.0.1");
		}

		bool is_valid_ip(const QString& ip)
		{
			QHostAddress address(ip);
			return !address.isNull() && address.protocol() == QAbstractSocket::IPv4Protocol;
		}

		bool is_valid_port(int port)
		{
			return port > 0 && port <= 65535;
		}

		QString get_socket_error_message(int error_code)
		{
			switch (error_code)
			{
				case QAbstractSocket::ConnectionRefusedError: return "Connection refused";
				case QAbstractSocket::RemoteHostClosedError: return "Remote host closed connection";
				case QAbstractSocket::HostNotFoundError: return "Host not found";
				case QAbstractSocket::SocketAccessError: return "Permission denied";
				case QAbstractSocket::SocketResourceError: return "Resource error";
				case QAbstractSocket::SocketTimeoutError: return "Connection timed out";
				case QAbstractSocket::NetworkError: return "Network error";
				case QAbstractSocket::AddressInUseError: return "Address already in use";
				case QAbstractSocket::SocketAddressNotAvailableError: return "Address not available";
				default: return "Unknown socket error: " + QString::number(error_code);
			}
		}

		QString format_client_address(const QString& ip, int port)
		{
			return ip + ":" + QString::number(port);
		}
	}

	namespace Logger
	{
		static Level current_log_level = Level::DEBUG;
		static bool file_logging_enabled = true;
		static bool logging_initialized = false;

		void log(Level level, const QString& message)
		{
			if (level < current_log_level)
				return;
			
			QString formatted_message = "[" + DateTime::get_current_date_time() + "] " + message;
			
			// Use Qt logging categories for better control
			switch (level)
			{
				case Level::DEBUG: 
					qCDebug(serverGeneral) << formatted_message;
					break;
				case Level::INFO: 
					qCInfo(serverGeneral) << formatted_message;
					break;
				case Level::WARNING: 
					qCWarning(serverGeneral) << formatted_message;
					break;
				case Level::ERR: 
					qCCritical(serverGeneral) << formatted_message;
					break;
				case Level::CRITICAL: 
					qCCritical(serverGeneral) << "CRITICAL:" << formatted_message;
					break;
			}
			
			// Output to file if enabled
			if (file_logging_enabled && logging_initialized)
			{
				log_to_file(get_log_filename(), formatted_message);
			}
		}

		void debug(const QString& message)
		{
			qCDebug(serverGeneral) << message;
			if (file_logging_enabled && logging_initialized)
			{
				log_to_file(get_log_filename(), "[DEBUG] " + message);
			}
		}

		void info(const QString& message)
		{
			qCInfo(serverGeneral) << message;
			if (file_logging_enabled && logging_initialized)
			{
				log_to_file(get_log_filename(), "[INFO] " + message);
			}
		}

		void warning(const QString& message)
		{
			qCWarning(serverGeneral) << message;
			if (file_logging_enabled && logging_initialized)
			{
				log_to_file(get_log_filename(), "[WARNING] " + message);
			}
		}

		void error(const QString& message)
		{
			qCCritical(serverGeneral) << message;
			if (file_logging_enabled && logging_initialized)
			{
				log_to_file(get_log_filename(), "[ERROR] " + message);
			}
		}

		void critical(const QString& message)
		{
			qCCritical(serverGeneral) << "CRITICAL:" << message;
			if (file_logging_enabled && logging_initialized)
			{
				log_to_file(get_log_filename(), "[CRITICAL] " + message);
			}
		}

		void set_log_level(Level level)
		{
			current_log_level = level;
		}

		void log_to_file(const QString& filename, const QString& message)
		{
			QFile file(filename);
			if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
			{
				QTextStream out(&file);
				out << "[" << DateTime::get_current_date_time() << "] " << message << "\n";
				file.close();
			}
			else
			{
				qCCritical(serverGeneral) << "Failed to open log file:" << filename;
			}
		}

		QString level_to_string(Level level)
		{
			switch (level)
			{
				case Level::DEBUG: return "DEBUG";
				case Level::INFO: return "INFO";
				case Level::WARNING: return "WARNING";
				case Level::ERR: return "ERROR";
				case Level::CRITICAL: return "CRITICAL";
				default: return "UNKNOWN";
			}
		}

		void initialize_logging()
		{
			try 
			{
				// Create logs directory if it doesn't exist
				if (!File::create_directory(Config::Application::LOG_DIRECTORY))
				{
					qCCritical(serverGeneral) << "Warning: Could not create logs directory";
				}
				
				logging_initialized = true;
				
				// Log startup message using Qt logging
				qCInfo(serverGeneral) << "=== Qt Logging system initialized ===";
				qCInfo(serverGeneral) << "Application:" << Config::Application::APP_NAME << "v" << Config::Application::VERSION;
				qCInfo(serverGeneral) << "Log directory:" << Config::Application::LOG_DIRECTORY;
				qCInfo(serverGeneral) << "System info:" << QSysInfo::prettyProductName() << QSysInfo::currentCpuArchitecture();
				
			}
			catch (const std::exception& e)
			{
				qCCritical(serverGeneral) << "Error initializing logging:" << e.what();
				logging_initialized = false;
			}
		}

		void enable_file_logging(bool enabled)
		{
			file_logging_enabled = enabled;
			if (logging_initialized)
			{
				QString status = enabled ? "enabled" : "disabled";
				qCInfo(serverGeneral) << "File logging" << status;
			}
		}

		QString get_log_filename()
		{
			QString date = DateTime::get_current_date();
			// Replace dashes with underscores for filename
			date.replace('-', '_');
			return Config::Application::LOG_DIRECTORY + "server_" + date + ".log";
		}
	}

	namespace JSON
	{
		bool is_valid_json(const QString& json_str)
		{
			QJsonParseError error;
			QJsonDocument::fromJson(json_str.toUtf8(), &error);
			return error.error == QJsonParseError::NoError;
		}

		QString escape_json(const QString& input)
		{
			QJsonDocument doc;
			QJsonObject obj;
			obj["value"] = input;
			doc.setObject(obj);
			QString escaped = doc.toJson(QJsonDocument::Compact);
			// Extract just the value part
			int start = escaped.indexOf(":") + 1;
			int end = escaped.lastIndexOf("}");
			return escaped.mid(start, end - start);
		}

		QString create_error_response(const QString& error_message, int error_code)
    	{
			QJsonObject response;
			response["success"] = false;
			response["message"] = error_message;
			if (error_code != -1)
				response["error_code"] = error_code;
			
			QJsonDocument doc(response);
			return doc.toJson(QJsonDocument::Compact);
    	}

		QString create_success_response(const QString& data, const QString& message)
   		{
			QJsonObject response;
			response["success"] = true;
			response["message"] = message.isEmpty() ? "Success" : message;
			
			if (!data.isEmpty())
			{
				QJsonParseError error;
				QJsonDocument dataDoc = QJsonDocument::fromJson(data.toUtf8(), &error);
				if (error.error == QJsonParseError::NoError)
				{
					if (dataDoc.isObject())
						response["data"] = dataDoc.object();
					else if (dataDoc.isArray())
						response["data"] = dataDoc.array();
					else
						response["data"] = data;
				}
				else
				{
					response["data"] = data;
				}
			}
			else
			{
				response["data"] = QJsonObject();
			}
			
			QJsonDocument doc(response);
			return doc.toJson(QJsonDocument::Compact);
    	}

		QString format_json(const QString& json_str)
		{
			QJsonParseError error;
			QJsonDocument doc = QJsonDocument::fromJson(json_str.toUtf8(), &error);
			if (error.error == QJsonParseError::NoError)
			{
				return doc.toJson(QJsonDocument::Indented);
			}
			return json_str; // Return original if parsing fails
		}
	}

	namespace Random
	{
		int generate_int(int min, int max)
		{
			return QRandomGenerator::global()->bounded(min, max + 1);
		}

		qreal generate_double(qreal min, qreal max)
		{
			return QRandomGenerator::global()->generateDouble() * (max - min) + min;
		}

		QString generate_random_string(size_t length, bool include_numbers, bool include_symbols)
		{
			QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
			if (include_numbers) chars += "0123456789";
			if (include_symbols) chars += "!@#$%^&*()_+-=";
			
			QString result;
			result.reserve(static_cast<int>(length));
			for (size_t i = 0; i < length; ++i) {
				int index = QRandomGenerator::global()->bounded(chars.size());
				result += chars[index];
			}
			return result;
		}

		QString generate_uuid()
		{
			return QUuid::createUuid().toString(QUuid::WithoutBraces);
		}
	}

	namespace Memory
	{
		qint64 get_memory_usage_MB()
		{
			// Qt doesn't provide direct process memory info, so we'll use a reasonable estimate
			// In a real application, you would use platform-specific APIs like Windows Task Manager API
			// or Linux /proc/self/status to get actual memory usage
			
			// For now, return a reasonable estimate based on a typical Qt application
			return 50; // Estimate 50MB for a typical server application
		}

		qint64 get_available_memory_MB()
		{
			// This is a placeholder - Qt doesn't provide system memory info directly
			// In production, you would use platform-specific APIs to get actual available RAM
			// For Windows: GlobalMemoryStatusEx(), for Linux: /proc/meminfo
			
			// Return a reasonable estimate for development
			return 4096; // Assume 4GB available memory
		}

		void log_memory_usage(const QString& context)
		{
			qint64 memory_usage = get_memory_usage_MB();
			qint64 available_memory = get_available_memory_MB();
			qCInfo(serverMemory) << context << "- Estimated Memory Usage:" << memory_usage << "MB, Available:" << available_memory << "MB";
			
			// Also log system storage info for disk monitoring
			QStorageInfo storageInfo = QStorageInfo::root();
			qint64 totalMB = storageInfo.bytesTotal() / (1024 * 1024);
			qint64 availableMB = storageInfo.bytesAvailable() / (1024 * 1024);
			qCInfo(serverMemory) << "System Storage - Total:" << totalMB << "MB, Available:" << availableMB << "MB";
		}

		void log_system_info()
		{
			qCInfo(serverGeneral) << "=== System Information ===";
			qCInfo(serverGeneral) << "OS:" << QSysInfo::prettyProductName();
			qCInfo(serverGeneral) << "Kernel:" << QSysInfo::kernelType() << QSysInfo::kernelVersion();
			qCInfo(serverGeneral) << "Architecture:" << QSysInfo::currentCpuArchitecture();
			qCInfo(serverGeneral) << "Build ABI:" << QSysInfo::buildAbi();
			qCInfo(serverGeneral) << "Qt Version:" << QT_VERSION_STR;
			
			// Storage information
			QStorageInfo storageInfo = QStorageInfo::root();
			qint64 totalGB = storageInfo.bytesTotal() / (1024 * 1024 * 1024);
			qint64 availableGB = storageInfo.bytesAvailable() / (1024 * 1024 * 1024);
			qCInfo(serverGeneral) << "Root Storage:" << totalGB << "GB total," << availableGB << "GB available";
		}
	}

	namespace Performance
	{
		Timer::Timer(const QString& name)
			: operation_name(name)
		{
			start_time = QDateTime::currentDateTime();
			elapsed_timer.start();
			qCDebug(serverPerformance) << "Timer started for:" << name;
		}

		Timer::~Timer()
		{
			qint64 elapsed = elapsed_timer.elapsed();
			qCInfo(serverPerformance) << "Timer ended for:" << operation_name << "- Duration:" << elapsed << "ms";
		}

		void Timer::reset()
		{
			start_time = QDateTime::currentDateTime();
			elapsed_timer.restart();
			qCDebug(serverPerformance) << "Timer reset for:" << operation_name;
		}

		qreal Timer::elapsed_milliseconds() const
		{
			return static_cast<qreal>(elapsed_timer.elapsed());
		}

		void start_operation(const QString& operation_name)
		{
			qCDebug(serverPerformance) << "Starting operation:" << operation_name;
			Timer timer(operation_name);
			timer.reset();
		}

		void end_operation(const QString& operation_name)
		{
			qCDebug(serverPerformance) << "Ending operation:" << operation_name;
			// Timer will automatically log the duration in its destructor
		}

		void log_performance(const QString& operation, qreal duration_ms)
		{
			qCInfo(serverPerformance) << "Performance - Operation:" << operation << "- Duration:" << duration_ms << "ms";
		}
	}
}
