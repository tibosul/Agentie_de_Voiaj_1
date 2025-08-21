#pragma once

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QDateTime>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <memory>
#include <functional>

// Utils header
#include "utils/utils.h"

// Data structures - included from separate header files
#include "models/All_Data_Structures.h"

namespace Database
{
	enum class Result_Type
	{
		SUCCESS,
		ERROR_CONNECTION,
		ERROR_EXECUTION,
		DB_ERROR_NO_DATA,
		ERROR_CONSTRAINT,
		DB_ERROR_TIMEOUT
	};

	struct Query_Result
	{
		Result_Type type;
		QString message;
		QList<QHash<QString, QVariant>> data;
		int affected_rows = 0;
		
		Query_Result(Result_Type t = Result_Type::SUCCESS, const QString& msg = "")
			: type(t), message(msg), affected_rows(0) 
		{
		}

		bool is_success() const 
		{ 
			return type == Result_Type::SUCCESS;
		}
		
		bool has_data() const 
		{ 
			return !data.isEmpty(); 
		}
	};

	class Database_Manager
	{
	private:
		QSqlDatabase db;

		QString server;
		QString database;
		QString username;
		QString password;
		QString connection_string;

		bool is_connected;
		bool is_demo_mode; // When true, returns mock data instead of real DB operations
		QMutex db_mutex;

		static constexpr int MAX_RETRIES_ATTEMPTS = 3;
		static constexpr int RETRY_DELAY_MS = 1000;

	public:
		Database_Manager();
		explicit Database_Manager(const QString& server, const QString& database, 
			const QString& username, const QString& password);
		~Database_Manager();

		// Connection methods
		bool connect();
		bool connect(const QString& server, const QString& database,
			const QString& username, const QString& password);
		bool disconnect();
		bool is_connection_alive() const;
		bool database_exists() const;
		bool reconnect();

		// Configuration
		void set_configuration_params(const QString& server, const QString& database,
			const QString& username, const QString& password);
		QString get_connection_string() const;

		// Core query methods
		Query_Result execute_query(const QString& query);
		Query_Result execute_select(const QString& query);
		Query_Result execute_insert(const QString& query);
		Query_Result execute_update(const QString& query);
		Query_Result execute_delete(const QString& query);

		// Advanced features
		Query_Result execute_prepared(const QString& query, const QHash<QString, QVariant>& params);
		
		// Transaction support
		bool begin_transaction();
		bool commit_transaction();
		bool rollback_transaction();
		Query_Result execute_transaction(const QStringList& queries);

		// Stored procedures
		Query_Result execute_stored_procedure(const QString& procedure_name, const QStringList& params);
		
		// Schema operations
		bool table_exists(const QString& table_name);
		QStringList get_table_columns(const QString& table_name);
		bool create_tables_if_not_exists();

		// User management
		Query_Result authenticate_user(const QString& username, const QString& password);
		Query_Result register_user(const User_Data& user_data);
		Query_Result get_user_by_id(int user_id);
		Query_Result get_user_by_username(const QString& username);
		Query_Result update_user(const User_Data& user);
		Query_Result delete_user(int user_id);
		Query_Result change_password(int user_id, const QString& old_password,
			const QString& new_password);

		// Destination management
		Query_Result get_all_destinations();
		Query_Result get_destination_by_id(int destination_id);
		Query_Result add_destination(const Destination_Data& destination);
		Query_Result update_destination(const Destination_Data& destination);
		Query_Result delete_destination(int destination_id);

		// Transport types management
		Query_Result get_all_transport_types();
		Query_Result get_transport_type_by_id(int transport_type_id);
		Query_Result add_transport_type(const Transport_Type_Data& transport_type);
		Query_Result update_transport_type(const Transport_Type_Data& transport_type);
		Query_Result delete_transport_type(int transport_type_id);

		// Accommodation types management
		Query_Result get_all_accommodation_types();
		Query_Result get_accommodation_type_by_id(int accommodation_type_id);
		Query_Result add_accommodation_type(const Accommodation_Type_Data& accommodation_type);
		Query_Result update_accommodation_type(const Accommodation_Type_Data& accommodation_type);
		Query_Result delete_accommodation_type(int accommodation_type_id);

		// Accommodation management
		Query_Result get_accommodations_by_destination(int destination_id);
		Query_Result get_accommodation_by_id(int accommodation_id);
		Query_Result add_accommodation(const Accommodation_Data& accommodation);
		Query_Result update_accommodation(const Accommodation_Data& accommodation);
		Query_Result delete_accommodation(int accommodation_id);

		// Offer management
		Query_Result get_all_offers();
		Query_Result get_available_offers();
		Query_Result get_offer_by_id(int offer_id);
		Query_Result search_offers(const QString& destination = "", 
			qreal min_price = 0, qreal max_price = 0,
			const QString& start_date = "", 
			const QString& end_date = "");
		Query_Result add_offer(const Offer_Data& offer);
		Query_Result update_offer(const Offer_Data& offer);
		Query_Result delete_offer(int offer_id);

		// Reservation management
		Query_Result book_offer(int user_id, int offer_id, int person_count = 1);
		Query_Result get_user_reservations(int user_id);
		Query_Result get_offer_reservations(int offer_id);
		Query_Result get_reservation_by_id(int reservation_id);
		Query_Result cancel_reservation(int reservation_id);
		Query_Result update_reservation_status(int reservation_id, const QString& status);

		// Reservation persons
		Query_Result add_reservation_person(const Reservation_Person_Data& person_data);
		Query_Result get_reservation_persons(int reservation_id);
		Query_Result update_reservation_person(const Reservation_Person_Data& person_data);
		Query_Result delete_reservation_person(int person_id);

		// Statistics
		Query_Result get_popular_destinations(int limit = 10);
		Query_Result get_revenue_report(const QString& start_date, const QString& end_date);
		Query_Result get_user_statistics();
		Query_Result get_booking_statistics();

		// Utilities
		QString escape_string(const QString& input);
		QString format_date_for_sql(const QString& date);
		bool validate_connection_params();
		QString get_last_error();
		void log_error(const QString& operation, const QString& error);
		
		// Demo mode utilities
		void enable_demo_mode();
		bool is_running_in_demo_mode() const;
		Query_Result create_mock_response(const QString& operation);

		// Static utilities
		static QString hash_password(const QString& password, const QString& salt);
		static QString generate_salt();
		static bool validate_email(const QString& email);
		static bool validate_cnp(const QString& cnp);

	private:
		// Private helpers
		bool initialize_qt_sql();
		void cleanup_qt_sql();
		QString build_connection_string() const;
		Query_Result process_select_result(QSqlQuery& query);
		Query_Result process_execution_result(QSqlQuery& query);
		bool handle_sql_error(const QSqlError& error);
		QString get_sql_error(const QSqlError& error);
		bool retry_operation(std::function<bool()> operation, int max_attempts = MAX_RETRIES_ATTEMPTS);
		
		// Table creation SQL
		QString get_create_users_table_sql();
		QString get_create_destinations_table_sql();
		QString get_create_transport_types_table_sql();
		QString get_create_accommodation_types_table_sql();
		QString get_create_accommodations_table_sql();
		QString get_create_offers_table_sql();
		QString get_create_reservations_table_sql();
		QString get_create_reservation_persons_table_sql();
		QString get_create_indexes_sql();
	};
}