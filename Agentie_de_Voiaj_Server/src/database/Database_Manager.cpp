#include "database/Database_Manager.h"
#include <QCoreApplication>
#include <QSqlDriver>
#include <QDebug>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QThread>
#include <chrono>
#include <thread>

using namespace Database;

// Constructor
Database_Manager::Database_Manager() 
    : is_connected(false), is_demo_mode(false)
{
    initialize_qt_sql();
}

Database_Manager::Database_Manager(const QString& server, const QString& database, 
    const QString& username, const QString& password)
    : server(server), database(database), username(username), password(password),
      is_connected(false), is_demo_mode(false)
{
    // Check if this is a dummy instance (demo mode)
    if (server == "dummy" && database == "dummy")
    {
        is_demo_mode = true;
        qWarning() << "Database_Manager initialized in DEMO MODE - using mock data";
        return;
    }
    
    initialize_qt_sql();
    connection_string = build_connection_string();
}

// Destructor
Database_Manager::~Database_Manager()
{
    disconnect();
    cleanup_qt_sql();
}

// Initialize Qt SQL
bool Database_Manager::initialize_qt_sql()
{
    if (!QSqlDatabase::isDriverAvailable("QODBC"))
    {
        log_error("initialize_qt_sql", "QODBC driver not available");
        return false;
    }

    // Create unique connection name for this instance
    QString connection_name = QString("db_conn_%1_%2").arg(
        reinterpret_cast<quintptr>(this)).arg(QDateTime::currentMSecsSinceEpoch());
    
    db = QSqlDatabase::addDatabase("QODBC", connection_name);
    return true;
}

// Cleanup Qt SQL
void Database_Manager::cleanup_qt_sql()
{
    if (db.isValid())
    {
        QString connection_name = db.connectionName();
        db.close();
        QSqlDatabase::removeDatabase(connection_name);
    }
}

// Build connection string
QString Database_Manager::build_connection_string() const
{
    QString conn_str = "DRIVER={ODBC Driver 17 for SQL Server};";
    conn_str += QString("SERVER=%1;").arg(server);
    
    // Use Windows Authentication if username/password are empty
    if (username.isEmpty() && password.isEmpty())
    {
        conn_str += QString("DATABASE=%1;").arg(database);
        conn_str += "Trusted_Connection=yes;";
        conn_str += "Connection Timeout=30;";
    }
    else
    {
        conn_str += QString("DATABASE=%1;").arg(database);
        conn_str += QString("UID=%1;").arg(username);
        conn_str += QString("PWD=%1;").arg(password);
        conn_str += "Trusted_Connection=no;";
        conn_str += "Connection Timeout=30;";
    }
    
    return conn_str;
}

// Connection methods
bool Database_Manager::connect()
{
    QMutexLocker locker(&db_mutex);
    
    if (is_connected)
    {
        return true;
    }

    if (connection_string.isEmpty())
    {
        log_error("connect", "Connection string is empty");
        return false;
    }

    db.setDatabaseName(connection_string);
    
    if (db.open())
    {
        is_connected = true;
        qInfo() << "Database connection successful to:" << server << "\\" << database;
        return true;
    }
    else
    {
        QString error = QString("Connection failed to %1\\%2: %3")
                       .arg(server, database, db.lastError().text());
        qCritical() << error;
        log_error("connect", error);
        return false;
    }
}

bool Database_Manager::connect(const QString& server, const QString& database,
    const QString& username, const QString& password)
{
    set_configuration_params(server, database, username, password);
    return connect();
}

bool Database_Manager::disconnect()
{
    QMutexLocker locker(&db_mutex);
    
    if (!is_connected)
    {
        return true;
    }

    db.close();
    is_connected = false;
    
    return true;
}

bool Database_Manager::is_connection_alive() const
{
    if (!is_connected || !db.isOpen()) return false;
    
    // Test connection with a simple query
    QSqlQuery test_query(db);
    return test_query.exec("SELECT 1");
}

bool Database_Manager::database_exists() const
{
    if (!is_connected) return false;
    
    QSqlQuery query(db);
    return query.exec("SELECT 1");
}

bool Database_Manager::reconnect()
{
    disconnect();
    return connect();
}

void Database_Manager::set_configuration_params(const QString& server, const QString& database,
    const QString& username, const QString& password)
{
    this->server = server;
    this->database = database;
    this->username = username;
    this->password = password;
    this->connection_string = build_connection_string();
}

QString Database_Manager::get_connection_string() const
{
    return connection_string;
}

// Core query methods
Query_Result Database_Manager::execute_query(const QString& query)
{
    QMutexLocker locker(&db_mutex);
    
    if (!is_connected)
    {
        return Query_Result(Result_Type::ERROR_CONNECTION, "Not connected to database");
    }

    QSqlQuery sql_query(db);
    if (!sql_query.exec(query))
    {
        QString error = get_sql_error(sql_query.lastError());
        log_error("execute_query", error);
        return Query_Result(Result_Type::ERROR_EXECUTION, error);
    }

    // Check if this is a SELECT query
    QString upper_query = query.toUpper().trimmed();
    
    if (upper_query.startsWith("SELECT"))
    {
        return process_select_result(sql_query);
    }
    else
    {
        return process_execution_result(sql_query);
    }
}

Query_Result Database_Manager::execute_select(const QString& query)
{
    return execute_query(query);
}

Query_Result Database_Manager::execute_insert(const QString& query)
{
    return execute_query(query);
}

Query_Result Database_Manager::execute_update(const QString& query)
{
    return execute_query(query);
}

Query_Result Database_Manager::execute_delete(const QString& query)
{
    return execute_query(query);
}

// Advanced features
Query_Result Database_Manager::execute_prepared(const QString& query, const QHash<QString, QVariant>& params)
{
    QMutexLocker locker(&db_mutex);
    
    if (!is_connected)
    {
        return Query_Result(Result_Type::ERROR_CONNECTION, "Not connected to database");
    }

    QSqlQuery sql_query(db);
    sql_query.prepare(query);
    
    // Bind parameters
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
    {
        sql_query.bindValue(it.key(), it.value());
    }
    
    if (!sql_query.exec())
    {
        QString error = get_sql_error(sql_query.lastError());
        log_error("execute_prepared", error);
        return Query_Result(Result_Type::ERROR_EXECUTION, error);
    }

    QString upper_query = query.toUpper().trimmed();
    if (upper_query.startsWith("SELECT"))
    {
        return process_select_result(sql_query);
    }
    else
    {
        return process_execution_result(sql_query);
    }
}

// Transaction support
bool Database_Manager::begin_transaction()
{
    QMutexLocker locker(&db_mutex);
    return db.transaction();
}

bool Database_Manager::commit_transaction()
{
    QMutexLocker locker(&db_mutex);
    return db.commit();
}

bool Database_Manager::rollback_transaction()
{
    QMutexLocker locker(&db_mutex);
    return db.rollback();
}

Query_Result Database_Manager::execute_transaction(const QStringList& queries)
{
    if (!begin_transaction())
    {
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to begin transaction");
    }

    for (const QString& query : queries)
    {
        Query_Result result = execute_query(query);
        if (!result.is_success())
        {
            rollback_transaction();
            return result;
        }
    }

    if (!commit_transaction())
    {
        rollback_transaction();
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to commit transaction");
    }

    return Query_Result(Result_Type::SUCCESS, "Transaction completed successfully");
}

// Stored procedures
Query_Result Database_Manager::execute_stored_procedure(const QString& procedure_name, const QStringList& params)
{
    QString query = QString("EXEC %1").arg(procedure_name);
    
    for (int i = 0; i < params.size(); ++i)
    {
        if (i > 0) query += ",";
        query += QString(" '%1'").arg(escape_string(params[i]));
    }
    
    return execute_query(query);
}

// Schema operations
bool Database_Manager::table_exists(const QString& table_name)
{
    QString query = QString("SELECT 1 FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = '%1'")
                   .arg(escape_string(table_name));
    Query_Result result = execute_select(query);
    return result.is_success() && result.has_data();
}

QStringList Database_Manager::get_table_columns(const QString& table_name)
{
    QStringList columns;
    QString query = QString("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = '%1' ORDER BY ORDINAL_POSITION")
                   .arg(escape_string(table_name));
    Query_Result result = execute_select(query);
    
    if (result.is_success())
    {
        for (const auto& row : result.data)
        {
            columns.append(row["COLUMN_NAME"].toString());
        }
    }
    
    return columns;
}

bool Database_Manager::create_tables_if_not_exists()
{
    QStringList create_queries = {
        get_create_users_table_sql(),
        get_create_destinations_table_sql(),
        get_create_transport_types_table_sql(),
        get_create_accommodation_types_table_sql(),
        get_create_accommodations_table_sql(),
        get_create_offers_table_sql(),
        get_create_reservations_table_sql(),
        get_create_reservation_persons_table_sql(),
        get_create_indexes_sql()
    };
    
    for (const QString& query : create_queries)
    {
        Query_Result result = execute_query(query);
        if (!result.is_success())
        {
            log_error("create_tables_if_not_exists", QString("Failed to execute: %1").arg(query));
            return false;
        }
    }
    
    return true;
}

// Process SELECT result
Query_Result Database_Manager::process_select_result(QSqlQuery& query)
{
    Query_Result result;
    
    while (query.next())
    {
        QHash<QString, QVariant> row;
        QSqlRecord record = query.record();
        
        for (int i = 0; i < record.count(); i++)
        {
            QString field_name = record.fieldName(i);
            QVariant value = query.value(i);
            row[field_name] = value;
        }
        
        result.data.append(row);
    }
    
    return result;
}

// Process non-SELECT result
Query_Result Database_Manager::process_execution_result(QSqlQuery& query)
{
    Query_Result result;
    result.affected_rows = query.numRowsAffected();
    return result;
}

// Error handling
bool Database_Manager::handle_sql_error(const QSqlError& error)
{
    QString error_text = get_sql_error(error);
    log_error("SQL Error", error_text);
    return false;
}

QString Database_Manager::get_sql_error(const QSqlError& error)
{
    return QString("SQL Error: %1 - %2").arg(error.nativeErrorCode(), error.text());
}

QString Database_Manager::get_last_error()
{
    return get_sql_error(db.lastError());
}

void Database_Manager::log_error(const QString& operation, const QString& error)
{
    qCritical() << QString("[Database] %1: %2").arg(operation, error);
}

// Utilities
QString Database_Manager::escape_string(const QString& input)
{
    QString escaped = input;
    escaped.replace("'", "''");
    return escaped;
}

QString Database_Manager::format_date_for_sql(const QString& date)
{
    return QString("'%1'").arg(date);
}

bool Database_Manager::validate_connection_params()
{
    return !server.isEmpty() && !database.isEmpty();
}

// Demo mode utilities
void Database_Manager::enable_demo_mode()
{
    is_demo_mode = true;
    qWarning() << "Database_Manager switched to DEMO MODE";
}

bool Database_Manager::is_running_in_demo_mode() const
{
    return is_demo_mode;
}

Query_Result Database_Manager::create_mock_response(const QString& operation)
{
    qInfo() << QString("DEMO MODE: Creating mock response for operation: %1").arg(operation);
    
    if (operation == "get_destinations")
    {
        Query_Result result(Result_Type::SUCCESS, "Demo destinations retrieved");
        
        // Mock destination 1
        QHash<QString, QVariant> dest1;
        dest1["Destination_ID"] = 1;
        dest1["Name"] = "Paris";
        dest1["Country"] = "France";
        dest1["Description"] = "City of Light - Demo destination";
        result.data.append(dest1);
        
        // Mock destination 2
        QHash<QString, QVariant> dest2;
        dest2["Destination_ID"] = 2;
        dest2["Name"] = "Rome";
        dest2["Country"] = "Italy";
        dest2["Description"] = "Eternal City - Demo destination";
        result.data.append(dest2);
        
        return result;
    }
    else if (operation == "get_offers")
    {
        Query_Result result(Result_Type::SUCCESS, "Demo offers retrieved");
        
        // Mock offer 1
        QHash<QString, QVariant> offer1;
        offer1["Offer_ID"] = 1;
        offer1["Name"] = "Paris Weekend";
        offer1["Destination"] = "Paris";
        offer1["Price_per_Person"] = 299.99;
        offer1["Duration_Days"] = 3;
        offer1["Available_Seats"] = 10;
        result.data.append(offer1);
        
        // Mock offer 2
        QHash<QString, QVariant> offer2;
        offer2["Offer_ID"] = 2;
        offer2["Name"] = "Rome Adventure";
        offer2["Destination"] = "Rome";
        offer2["Price_per_Person"] = 449.99;
        offer2["Duration_Days"] = 5;
        offer2["Available_Seats"] = 8;
        result.data.append(offer2);
        
        return result;
    }
    else
    {
        return Query_Result(Result_Type::SUCCESS, QString("Demo mode: Operation '%1' completed successfully").arg(operation));
    }
}

// Static utilities
QString Database_Manager::hash_password(const QString& password, const QString& salt)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData((password + salt).toUtf8());
    return hash.result().toHex();
}

QString Database_Manager::generate_salt()
{
    QByteArray salt;
    for (int i = 0; i < 32; ++i)
    {
        salt.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return salt.toHex();
}

bool Database_Manager::validate_email(const QString& email)
{
    return email.contains("@") && email.contains(".");
}

bool Database_Manager::validate_cnp(const QString& cnp)
{
    if (cnp.length() != 13) return false;
    
    bool ok;
    cnp.toLongLong(&ok);
    return ok;
}

// User management
Query_Result Database_Manager::authenticate_user(const QString& username, const QString& password)
{
    // Demo mode - return mock authentication
    if (is_demo_mode)
    {
        if (username.isEmpty() || password.isEmpty())
        {
            return Query_Result(Result_Type::ERROR_CONSTRAINT, "Invalid username or password format");
        }
        
        // Mock successful authentication for demo users
        if ((username == "admin" && password == "admin123") || 
            (username == "demo" && password == "demo123") ||
            (username == "test" && password == "test123"))
        {
            Query_Result result(Result_Type::SUCCESS, "Demo authentication successful");
            QHash<QString, QVariant> user_data;
            user_data["ID"] = (username == "admin") ? 1 : 2;
            user_data["Username"] = username;
            user_data["Email"] = username + "@demo.com";
            user_data["First_Name"] = "Demo";
            user_data["Last_Name"] = "User";
            user_data["Phone"] = "0700000000";
            result.data.append(user_data);
            
            qInfo() << QString("DEMO MODE: Authentication successful for demo user: %1").arg(username);
            return result;
        }
        else
        {
            qWarning() << QString("DEMO MODE: Authentication failed for user: %1").arg(username);
            return Query_Result(Result_Type::DB_ERROR_NO_DATA, "Authentication failed - invalid credentials");
        }
    }
    
    // Regular database authentication
    if (username.isEmpty() || password.isEmpty())
    {
        return Query_Result(Result_Type::ERROR_CONSTRAINT, "Invalid username or password format");
    }
    
    // First get the user's salt from database
    QString salt_query = QString("SELECT Password_Salt FROM Users WHERE Username = '%1'").arg(escape_string(username));
    Query_Result salt_result = execute_select(salt_query);
    if (!salt_result.is_success() || salt_result.data.isEmpty())
    {
        return Query_Result(Result_Type::DB_ERROR_NO_DATA, "Invalid username or password");
    }
    
    QString stored_salt = salt_result.data[0]["Password_Salt"].toString();
    QString hashed_password = hash_password(password, stored_salt);
    
    QString query = QString("SELECT User_ID, Username, Email, First_Name, Last_Name, Phone FROM Users WHERE Username = '%1' AND Password_Hash = '%2'")
                   .arg(escape_string(username), escape_string(hashed_password));
    return execute_select(query);
}

Query_Result Database_Manager::register_user(const User_Data& user_data)
{
    // Demo mode - return mock registration
    if (is_demo_mode)
    {
        // Basic validation
        if (!validate_email(user_data.email))
        {
            return Query_Result(Result_Type::ERROR_CONSTRAINT, "Invalid email format");
        }
        
        // Mock successful registration (simulate username already exists for some cases)
        if (user_data.username == "admin" || user_data.username == "test")
        {
            qWarning() << QString("DEMO MODE: Registration failed - username already exists: %1").arg(user_data.username);
            return Query_Result(Result_Type::ERROR_CONSTRAINT, "Username already exists");
        }
        
        qInfo() << QString("DEMO MODE: Registration successful for new user: %1").arg(user_data.username);
        return Query_Result(Result_Type::SUCCESS, "Demo user registration successful");
    }
    
    // Regular database registration - validate user data
    if (!validate_email(user_data.email))
    {
        return Query_Result(Result_Type::ERROR_CONSTRAINT, "Invalid email format");
    }
    
    // Generate a random salt for this user
    QString salt = generate_salt();
    QString hashed_password = hash_password(user_data.password_hash, salt);
    
    QString query = QString("INSERT INTO Users (Username, Password_Hash, Password_Salt, Email, First_Name, Last_Name, Phone) VALUES ('%1', '%2', '%3', '%4', '%5', '%6', '%7')")
                   .arg(escape_string(user_data.username),
                        escape_string(hashed_password),
                        escape_string(salt),
                        escape_string(user_data.email),
                        escape_string(user_data.first_name),
                        escape_string(user_data.last_name),
                        escape_string(user_data.phone_number));
    
    return execute_insert(query);
}

Query_Result Database_Manager::get_user_by_id(int user_id)
{
    QString query = QString("SELECT User_ID, Username, Email, First_Name, Last_Name, Phone, Date_Created, Date_Modified FROM Users WHERE User_ID = %1").arg(user_id);
    return execute_select(query);
}

Query_Result Database_Manager::get_user_by_username(const QString& username)
{
    QString query = QString("SELECT User_ID, Username, Email, First_Name, Last_Name, Phone, Date_Created, Date_Modified FROM Users WHERE Username = '%1'").arg(escape_string(username));
    return execute_select(query);
}

Query_Result Database_Manager::update_user(const User_Data& user)
{
    QString query = QString("UPDATE Users SET Email = '%1', First_Name = '%2', Last_Name = '%3', Phone = '%4', Date_Modified = GETDATE() WHERE User_ID = %5")
                   .arg(escape_string(user.email),
                        escape_string(user.first_name),
                        escape_string(user.last_name),
                        escape_string(user.phone_number))
                   .arg(user.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_user(int user_id)
{
    QString query = QString("DELETE FROM Users WHERE User_ID = %1").arg(user_id);
    return execute_delete(query);
}

Query_Result Database_Manager::change_password(int user_id, const QString& old_password, const QString& new_password)
{
    // First verify old password
    Query_Result user_result = get_user_by_id(user_id);
    if (!user_result.is_success() || user_result.data.isEmpty())
    {
        return Query_Result(Result_Type::DB_ERROR_NO_DATA, "User not found");
    }
    
    // Get current salt to verify old password
    QString current_salt_query = QString("SELECT Password_Salt FROM Users WHERE User_ID = %1").arg(user_id);
    Query_Result salt_result = execute_select(current_salt_query);
    if (!salt_result.is_success() || salt_result.data.isEmpty())
    {
        return Query_Result(Result_Type::DB_ERROR_NO_DATA, "User salt not found");
    }
    
    QString current_salt = salt_result.data[0]["Password_Salt"].toString();
    QString old_hash = hash_password(old_password, current_salt);
    
    QString verify_query = QString("SELECT User_ID FROM Users WHERE User_ID = %1 AND Password_Hash = '%2'")
                          .arg(user_id).arg(escape_string(old_hash));
    Query_Result verify_result = execute_select(verify_query);
    
    if (!verify_result.is_success() || verify_result.data.isEmpty())
    {
        return Query_Result(Result_Type::ERROR_EXECUTION, "Invalid old password");
    }
    
    // Generate new salt and hash for new password
    QString new_salt = generate_salt();
    QString new_hash = hash_password(new_password, new_salt);
    QString update_query = QString("UPDATE Users SET Password_Hash = '%1', Password_Salt = '%2', Date_Modified = GETDATE() WHERE User_ID = %3")
                          .arg(escape_string(new_hash), escape_string(new_salt)).arg(user_id);
    
    return execute_update(update_query);
}

// Destination management
Query_Result Database_Manager::get_all_destinations()
{
    QString query = "SELECT Destination_ID, Name, Country, Description, Image_Path, Date_Created, Date_Modified FROM Destinations ORDER BY Name";
    return execute_select(query);
}

Query_Result Database_Manager::get_destination_by_id(int destination_id)
{
    QString query = QString("SELECT Destination_ID, Name, Country, Description, Image_Path, Date_Created, Date_Modified FROM Destinations WHERE Destination_ID = %1").arg(destination_id);
    return execute_select(query);
}

Query_Result Database_Manager::add_destination(const Destination_Data& destination)
{
    QString query = QString("INSERT INTO Destinations (Name, Country, Description, Image_Path) VALUES ('%1', '%2', '%3', '%4')")
                   .arg(escape_string(destination.name),
                        escape_string(destination.country),
                        escape_string(destination.description),
                        escape_string(destination.image_path));
    
    return execute_insert(query);
}

Query_Result Database_Manager::update_destination(const Destination_Data& destination)
{
    QString query = QString("UPDATE Destinations SET Name = '%1', Country = '%2', Description = '%3', Image_Path = '%4', Date_Modified = GETDATE() WHERE Destination_ID = %5")
                   .arg(escape_string(destination.name),
                        escape_string(destination.country),
                        escape_string(destination.description),
                        escape_string(destination.image_path))
                   .arg(destination.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_destination(int destination_id)
{
    QString query = QString("DELETE FROM Destinations WHERE Destination_ID = %1").arg(destination_id);
    return execute_delete(query);
}

// Transport types management
Query_Result Database_Manager::get_all_transport_types()
{
    QString query = "SELECT Transport_Type_ID, Name, Description, Date_Created, Date_Modified FROM Types_of_Transport ORDER BY Name";
    return execute_select(query);
}

Query_Result Database_Manager::get_transport_type_by_id(int transport_type_id)
{
    QString query = QString("SELECT Transport_Type_ID, Name, Description, Date_Created, Date_Modified FROM Types_of_Transport WHERE Transport_Type_ID = %1").arg(transport_type_id);
    return execute_select(query);
}

Query_Result Database_Manager::add_transport_type(const Transport_Type_Data& transport_type)
{
    QString query = QString("INSERT INTO Types_of_Transport (Name, Description) VALUES ('%1', '%2')")
                   .arg(escape_string(transport_type.name),
                        escape_string(transport_type.description));
    
    return execute_insert(query);
}

Query_Result Database_Manager::update_transport_type(const Transport_Type_Data& transport_type)
{
    QString query = QString("UPDATE Types_of_Transport SET Name = '%1', Description = '%2', Date_Modified = GETDATE() WHERE Transport_Type_ID = %3")
                   .arg(escape_string(transport_type.name),
                        escape_string(transport_type.description))
                   .arg(transport_type.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_transport_type(int transport_type_id)
{
    QString query = QString("DELETE FROM Types_of_Transport WHERE Transport_Type_ID = %1").arg(transport_type_id);
    return execute_delete(query);
}

// Accommodation types management
Query_Result Database_Manager::get_all_accommodation_types()
{
    QString query = "SELECT Accommodation_Type_ID, Name, Description, Date_Created, Date_Modified FROM Types_of_Accommodation ORDER BY Name";
    return execute_select(query);
}

Query_Result Database_Manager::get_accommodation_type_by_id(int accommodation_type_id)
{
    QString query = QString("SELECT Accommodation_Type_ID, Name, Description, Date_Created, Date_Modified FROM Types_of_Accommodation WHERE Accommodation_Type_ID = %1").arg(accommodation_type_id);
    return execute_select(query);
}

Query_Result Database_Manager::add_accommodation_type(const Accommodation_Type_Data& accommodation_type)
{
    QString query = QString("INSERT INTO Types_of_Accommodation (Name, Description) VALUES ('%1', '%2')")
                   .arg(escape_string(accommodation_type.name),
                        escape_string(accommodation_type.description));
    
    return execute_insert(query);
}

Query_Result Database_Manager::update_accommodation_type(const Accommodation_Type_Data& accommodation_type)
{
    QString query = QString("UPDATE Types_of_Accommodation SET Name = '%1', Description = '%2', Date_Modified = GETDATE() WHERE Accommodation_Type_ID = %3")
                   .arg(escape_string(accommodation_type.name),
                        escape_string(accommodation_type.description))
                   .arg(accommodation_type.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_accommodation_type(int accommodation_type_id)
{
    QString query = QString("DELETE FROM Types_of_Accommodation WHERE Accommodation_Type_ID = %1").arg(accommodation_type_id);
    return execute_delete(query);
}

// Accommodation management
Query_Result Database_Manager::get_accommodations_by_destination(int destination_id)
{
    QString query = QString("SELECT a.Accommodation_ID, a.Name, a.Destination_ID, a.Type_of_Accommodation, "
                           "a.Category, a.Address, a.Facilities, a.Rating, a.Description, a.Date_Created, a.Date_Modified, "
                           "at.Name as Type_Name FROM Accommodations a "
                           "LEFT JOIN Types_of_Accommodation at ON a.Type_of_Accommodation = at.Accommodation_Type_ID "
                           "WHERE a.Destination_ID = %1 ORDER BY a.Name").arg(destination_id);
    return execute_select(query);
}

Query_Result Database_Manager::get_accommodation_by_id(int accommodation_id)
{
    QString query = QString("SELECT a.Accommodation_ID, a.Name, a.Destination_ID, a.Type_of_Accommodation, "
                           "a.Category, a.Address, a.Facilities, a.Rating, a.Description, a.Date_Created, a.Date_Modified, "
                           "at.Name as Type_Name FROM Accommodations a "
                           "LEFT JOIN Types_of_Accommodation at ON a.Type_of_Accommodation = at.Accommodation_Type_ID "
                           "WHERE a.Accommodation_ID = %1").arg(accommodation_id);
    return execute_select(query);
}

Query_Result Database_Manager::add_accommodation(const Accommodation_Data& accommodation)
{
    QString query = QString("INSERT INTO Accommodations (Name, Destination_ID, Type_of_Accommodation, Category, "
                           "Address, Facilities, Rating, Description) VALUES ('%1', %2, %3, '%4', '%5', '%6', %7, '%8')")
                   .arg(escape_string(accommodation.name))
                   .arg(accommodation.destination_id)
                   .arg(accommodation.accommodation_type_id)
                   .arg(escape_string(accommodation.category),
                        escape_string(accommodation.address),
                        escape_string(accommodation.facilities))
                   .arg(accommodation.rating)
                   .arg(escape_string(accommodation.description));
    
    return execute_insert(query);
}

Query_Result Database_Manager::update_accommodation(const Accommodation_Data& accommodation)
{
    QString query = QString("UPDATE Accommodations SET Name = '%1', Destination_ID = %2, Type_of_Accommodation = %3, "
                           "Category = '%4', Address = '%5', Facilities = '%6', Rating = %7, Description = '%8', "
                           "Date_Modified = GETDATE() WHERE Accommodation_ID = %9")
                   .arg(escape_string(accommodation.name))
                   .arg(accommodation.destination_id)
                   .arg(accommodation.accommodation_type_id)
                   .arg(escape_string(accommodation.category),
                        escape_string(accommodation.address),
                        escape_string(accommodation.facilities))
                   .arg(accommodation.rating)
                   .arg(escape_string(accommodation.description))
                   .arg(accommodation.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_accommodation(int accommodation_id)
{
    QString query = QString("DELETE FROM Accommodations WHERE Accommodation_ID = %1").arg(accommodation_id);
    return execute_delete(query);
}

// Offer management
Query_Result Database_Manager::get_all_offers()
{
    QString query = "SELECT o.Offer_ID, o.Name, o.Destination_ID, o.Accommodation_ID, o.Types_of_Transport_ID, "
                   "o.Price_per_Person, o.Duration_Days, o.Departure_Date, o.Return_Date, o.Total_Seats, "
                   "o.Reserved_Seats, o.Included_Services, o.Description, o.Status, o.Date_Created, o.Date_Modified, "
                   "d.Name as Destination_Name, d.Country, a.Name as Accommodation_Name, t.Name as Transport_Name "
                   "FROM Offers o "
                   "LEFT JOIN Destinations d ON o.Destination_ID = d.Destination_ID "
                   "LEFT JOIN Accommodations a ON o.Accommodation_ID = a.Accommodation_ID "
                   "LEFT JOIN Types_of_Transport t ON o.Types_of_Transport_ID = t.Transport_Type_ID "
                   "ORDER BY o.Departure_Date";
    return execute_select(query);
}

Query_Result Database_Manager::get_available_offers()
{
    QString query = "SELECT o.Offer_ID, o.Name, o.Destination_ID, o.Accommodation_ID, o.Types_of_Transport_ID, "
                   "o.Price_per_Person, o.Duration_Days, o.Departure_Date, o.Return_Date, o.Total_Seats, "
                   "o.Reserved_Seats, o.Included_Services, o.Description, o.Status, o.Date_Created, o.Date_Modified, "
                   "d.Name as Destination_Name, d.Country, a.Name as Accommodation_Name, t.Name as Transport_Name "
                   "FROM Offers o "
                   "LEFT JOIN Destinations d ON o.Destination_ID = d.Destination_ID "
                   "LEFT JOIN Accommodations a ON o.Accommodation_ID = a.Accommodation_ID "
                   "LEFT JOIN Types_of_Transport t ON o.Types_of_Transport_ID = t.Transport_Type_ID "
                   "WHERE o.Status = 'active' AND o.Reserved_Seats < o.Total_Seats AND o.Departure_Date > GETDATE() "
                   "ORDER BY o.Departure_Date";
    return execute_select(query);
}

Query_Result Database_Manager::get_offer_by_id(int offer_id)
{
    QString query = QString("SELECT o.Offer_ID, o.Name, o.Destination_ID, o.Accommodation_ID, o.Types_of_Transport_ID, "
                           "o.Price_per_Person, o.Duration_Days, o.Departure_Date, o.Return_Date, o.Total_Seats, "
                           "o.Reserved_Seats, o.Included_Services, o.Description, o.Status, o.Date_Created, o.Date_Modified, "
                           "d.Name as Destination_Name, d.Country, a.Name as Accommodation_Name, t.Name as Transport_Name "
                           "FROM Offers o "
                           "LEFT JOIN Destinations d ON o.Destination_ID = d.Destination_ID "
                           "LEFT JOIN Accommodations a ON o.Accommodation_ID = a.Accommodation_ID "
                           "LEFT JOIN Types_of_Transport t ON o.Types_of_Transport_ID = t.Transport_Type_ID "
                           "WHERE o.Offer_ID = %1").arg(offer_id);
    return execute_select(query);
}

Query_Result Database_Manager::search_offers(const QString& destination, qreal min_price, qreal max_price,
    const QString& start_date, const QString& end_date)
{
    QString query = "SELECT o.Offer_ID, o.Name, o.Destination_ID, o.Accommodation_ID, o.Types_of_Transport_ID, "
                   "o.Price_per_Person, o.Duration_Days, o.Departure_Date, o.Return_Date, o.Total_Seats, "
                   "o.Reserved_Seats, o.Included_Services, o.Description, o.Status, o.Date_Created, o.Date_Modified, "
                   "d.Name as Destination_Name, d.Country, a.Name as Accommodation_Name, t.Name as Transport_Name "
                   "FROM Offers o "
                   "LEFT JOIN Destinations d ON o.Destination_ID = d.Destination_ID "
                   "LEFT JOIN Accommodations a ON o.Accommodation_ID = a.Accommodation_ID "
                   "LEFT JOIN Types_of_Transport t ON o.Types_of_Transport_ID = t.Transport_Type_ID "
                   "WHERE o.Status = 'active' AND o.Reserved_Seats < o.Total_Seats";

    if (!destination.isEmpty())
    {
        query += QString(" AND (d.Name LIKE '%%1%' OR d.Country LIKE '%%1%')").arg(escape_string(destination));
    }
    
    if (min_price > 0)
    {
        query += QString(" AND o.Price_per_Person >= %1").arg(min_price);
    }
    
    if (max_price > 0)
    {
        query += QString(" AND o.Price_per_Person <= %1").arg(max_price);
    }
    
    if (!start_date.isEmpty())
    {
        query += QString(" AND o.Departure_Date >= '%1'").arg(escape_string(start_date));
    }
    
    if (!end_date.isEmpty())
    {
        query += QString(" AND o.Return_Date <= '%1'").arg(escape_string(end_date));
    }
    
    query += " ORDER BY o.Departure_Date";
    
    return execute_select(query);
}

Query_Result Database_Manager::add_offer(const Offer_Data& offer)
{
    QString query = QString("INSERT INTO Offers (Name, Destination_ID, Accommodation_ID, Types_of_Transport_ID, "
                           "Price_per_Person, Duration_Days, Departure_Date, Return_Date, Total_Seats, Reserved_Seats, "
                           "Included_Services, Description, Status) VALUES ('%1', %2, %3, %4, %5, %6, '%7', '%8', %9, %10, '%11', '%12', '%13')")
                   .arg(escape_string(offer.name))
                   .arg(offer.destination_id)
                   .arg(offer.accommodation_id)
                   .arg(offer.transport_type_id)
                   .arg(offer.price_per_person)
                   .arg(offer.duration_days)
                   .arg(escape_string(offer.departure_date),
                        escape_string(offer.return_date))
                   .arg(offer.total_seats)
                   .arg(offer.reserved_seats)
                   .arg(escape_string(offer.included_services),
                        escape_string(offer.description),
                        escape_string(offer.status));
    
    return execute_insert(query);
}

Query_Result Database_Manager::update_offer(const Offer_Data& offer)
{
    QString query = QString("UPDATE Offers SET Name = '%1', Destination_ID = %2, Accommodation_ID = %3, "
                           "Types_of_Transport_ID = %4, Price_per_Person = %5, Duration_Days = %6, "
                           "Departure_Date = '%7', Return_Date = '%8', Total_Seats = %9, Reserved_Seats = %10, "
                           "Included_Services = '%11', Description = '%12', Status = '%13', Date_Modified = GETDATE() "
                           "WHERE Offer_ID = %14")
                   .arg(escape_string(offer.name))
                   .arg(offer.destination_id)
                   .arg(offer.accommodation_id)
                   .arg(offer.transport_type_id)
                   .arg(offer.price_per_person)
                   .arg(offer.duration_days)
                   .arg(escape_string(offer.departure_date),
                        escape_string(offer.return_date))
                   .arg(offer.total_seats)
                   .arg(offer.reserved_seats)
                   .arg(escape_string(offer.included_services),
                        escape_string(offer.description),
                        escape_string(offer.status))
                   .arg(offer.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_offer(int offer_id)
{
    QString query = QString("DELETE FROM Offers WHERE Offer_ID = %1").arg(offer_id);
    return execute_delete(query);
}

// Reservation management
Query_Result Database_Manager::book_offer(int user_id, int offer_id, int person_count)
{
    // Begin transaction FIRST to ensure atomic operation
    if (!begin_transaction())
    {
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to begin transaction");
    }
    
    // ATOMICALLY check availability and reserve seats with row locking
    QString lock_query = QString("SELECT Total_Seats, Reserved_Seats, Price_per_Person "
                                 "FROM Offers WITH (UPDLOCK, ROWLOCK) WHERE Offer_ID = %1").arg(offer_id);
    
    Query_Result offer_result = execute_query(lock_query);
    if (!offer_result.is_success() || offer_result.data.isEmpty())
    {
        rollback_transaction();
        return Query_Result(Result_Type::DB_ERROR_NO_DATA, "Offer not found");
    }
    
    auto offer_data = offer_result.data[0];
    int total_seats = offer_data["Total_Seats"].toInt();
    int reserved_seats = offer_data["Reserved_Seats"].toInt();
    int available_seats = total_seats - reserved_seats;
    
    // Check seat availability within the locked transaction
    if (person_count > available_seats)
    {
        rollback_transaction();
        return Query_Result(Result_Type::ERROR_CONSTRAINT, "Not enough available seats");
    }
    
    qreal price_per_person = offer_data["Price_per_Person"].toReal();
    qreal total_price = price_per_person * person_count;
    
    // Insert reservation first
    QString insert_query = QString("INSERT INTO Reservations (User_ID, Offer_ID, Number_of_Persons, Total_Price, Status) VALUES (%1, %2, %3, %4, 'pending')")
                          .arg(user_id).arg(offer_id).arg(person_count).arg(total_price);
    
    Query_Result insert_result = execute_query(insert_query);
    if (!insert_result.is_success())
    {
        rollback_transaction();
        return insert_result;
    }
    
    // Update offer reserved seats with constraint check in SQL
    QString update_query = QString("UPDATE Offers SET Reserved_Seats = Reserved_Seats + %1 "
                                  "WHERE Offer_ID = %2 AND Reserved_Seats + %1 <= Total_Seats")
                          .arg(person_count).arg(offer_id);
    
    Query_Result update_result = execute_query(update_query);
    if (!update_result.is_success())
    {
        rollback_transaction();
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to update reserved seats");
    }
    
    // Verify the update affected a row (seat constraint was satisfied)
    if (update_result.affected_rows == 0)
    {
        rollback_transaction();
        return Query_Result(Result_Type::ERROR_CONSTRAINT, "Not enough available seats - concurrent booking detected");
    }
    
    if (!commit_transaction())
    {
        rollback_transaction();
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to commit transaction");
    }
    
    return Query_Result(Result_Type::SUCCESS, "Booking created successfully");
}

Query_Result Database_Manager::get_user_reservations(int user_id)
{
    QString query = QString("SELECT r.Reservation_ID, r.User_ID, r.Offer_ID, r.Number_of_Persons, r.Total_Price, "
                           "r.Reservation_Date, r.Status, r.Notes, "
                           "o.Name as Offer_Name, d.Name as Destination_Name, d.Country "
                           "FROM Reservations r "
                           "LEFT JOIN Offers o ON r.Offer_ID = o.Offer_ID "
                           "LEFT JOIN Destinations d ON o.Destination_ID = d.Destination_ID "
                           "WHERE r.User_ID = %1 ORDER BY r.Reservation_Date DESC").arg(user_id);
    return execute_select(query);
}

Query_Result Database_Manager::get_offer_reservations(int offer_id)
{
    QString query = QString("SELECT r.Reservation_ID, r.User_ID, r.Offer_ID, r.Number_of_Persons, r.Total_Price, "
                           "r.Reservation_Date, r.Status, r.Notes, "
                           "u.Username, u.First_Name, u.Last_Name, u.Email "
                           "FROM Reservations r "
                           "LEFT JOIN Users u ON r.User_ID = u.User_ID "
                           "WHERE r.Offer_ID = %1 ORDER BY r.Reservation_Date").arg(offer_id);
    return execute_select(query);
}

Query_Result Database_Manager::get_reservation_by_id(int reservation_id)
{
    QString query = QString("SELECT r.Reservation_ID, r.User_ID, r.Offer_ID, r.Number_of_Persons, r.Total_Price, "
                           "r.Reservation_Date, r.Status, r.Notes, "
                           "o.Name as Offer_Name, d.Name as Destination_Name, d.Country, "
                           "u.Username, u.First_Name, u.Last_Name, u.Email "
                           "FROM Reservations r "
                           "LEFT JOIN Offers o ON r.Offer_ID = o.Offer_ID "
                           "LEFT JOIN Destinations d ON o.Destination_ID = d.Destination_ID "
                           "LEFT JOIN Users u ON r.User_ID = u.User_ID "
                           "WHERE r.Reservation_ID = %1").arg(reservation_id);
    return execute_select(query);
}

Query_Result Database_Manager::cancel_reservation(int reservation_id)
{
    // Get reservation details first
    Query_Result reservation_result = get_reservation_by_id(reservation_id);
    if (!reservation_result.is_success() || reservation_result.data.isEmpty())
    {
        return Query_Result(Result_Type::DB_ERROR_NO_DATA, "Reservation not found");
    }
    
    auto reservation_data = reservation_result.data[0];
    int offer_id = reservation_data["Offer_ID"].toInt();
    int person_count = reservation_data["Number_of_Persons"].toInt();
    QString current_status = reservation_data["Status"].toString();
    
    if (current_status == "cancelled")
    {
        return Query_Result(Result_Type::ERROR_CONSTRAINT, "Reservation already cancelled");
    }
    
    // Begin transaction
    if (!begin_transaction())
    {
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to begin transaction");
    }
    
    // Update reservation status
    QString update_reservation_query = QString("UPDATE Reservations SET Status = 'cancelled' WHERE Reservation_ID = %1").arg(reservation_id);
    Query_Result update_result = execute_query(update_reservation_query);
    if (!update_result.is_success())
    {
        rollback_transaction();
        return update_result;
    }
    
    // Update offer available seats
    QString update_offer_query = QString("UPDATE Offers SET Reserved_Seats = Reserved_Seats - %1 WHERE Offer_ID = %2")
                                .arg(person_count).arg(offer_id);
    
    Query_Result seats_result = execute_query(update_offer_query);
    if (!seats_result.is_success())
    {
        rollback_transaction();
        return seats_result;
    }
    
    if (!commit_transaction())
    {
        rollback_transaction();
        return Query_Result(Result_Type::ERROR_EXECUTION, "Failed to commit transaction");
    }
    
    return Query_Result(Result_Type::SUCCESS, "Reservation cancelled successfully");
}

Query_Result Database_Manager::update_reservation_status(int reservation_id, const QString& status)
{
    QString query = QString("UPDATE Reservations SET Status = '%1' WHERE Reservation_ID = %2")
                   .arg(escape_string(status)).arg(reservation_id);
    return execute_update(query);
}

// Reservation persons
Query_Result Database_Manager::add_reservation_person(const Reservation_Person_Data& person_data)
{
    QString query = QString("INSERT INTO Reservation_Persons (Reservation_ID, Full_Name, CNP, Birth_Date, Person_Type) VALUES (%1, '%2', '%3', '%4', '%5')")
                   .arg(person_data.reservation_id)
                   .arg(escape_string(person_data.full_name),
                        escape_string(person_data.cnp),
                        escape_string(person_data.birth_date),
                        escape_string(person_data.person_type));
    
    return execute_insert(query);
}

Query_Result Database_Manager::get_reservation_persons(int reservation_id)
{
    QString query = QString("SELECT Reservation_Person_ID, Reservation_ID, Full_Name, CNP, Birth_Date, Person_Type "
                           "FROM Reservation_Persons WHERE Reservation_ID = %1 ORDER BY Reservation_Person_ID").arg(reservation_id);
    return execute_select(query);
}

Query_Result Database_Manager::update_reservation_person(const Reservation_Person_Data& person_data)
{
    QString query = QString("UPDATE Reservation_Persons SET Full_Name = '%1', CNP = '%2', Birth_Date = '%3', Person_Type = '%4' WHERE Reservation_Person_ID = %5")
                   .arg(escape_string(person_data.full_name),
                        escape_string(person_data.cnp),
                        escape_string(person_data.birth_date),
                        escape_string(person_data.person_type))
                   .arg(person_data.id);
    
    return execute_update(query);
}

Query_Result Database_Manager::delete_reservation_person(int person_id)
{
    QString query = QString("DELETE FROM Reservation_Persons WHERE Reservation_Person_ID = %1").arg(person_id);
    return execute_delete(query);
}

// Statistics
Query_Result Database_Manager::get_popular_destinations(int limit)
{
    QString query = QString("SELECT TOP %1 d.Destination_ID, d.Name, d.Country, COUNT(r.Reservation_ID) as Booking_Count "
                           "FROM Destinations d "
                           "LEFT JOIN Offers o ON d.Destination_ID = o.Destination_ID "
                           "LEFT JOIN Reservations r ON o.Offer_ID = r.Offer_ID AND r.Status != 'cancelled' "
                           "GROUP BY d.Destination_ID, d.Name, d.Country "
                           "ORDER BY Booking_Count DESC").arg(limit);
    return execute_select(query);
}

Query_Result Database_Manager::get_revenue_report(const QString& start_date, const QString& end_date)
{
    QString query = "SELECT COUNT(r.Reservation_ID) as Total_Reservations, "
                   "SUM(r.Total_Price) as Total_Revenue, "
                   "AVG(r.Total_Price) as Average_Booking_Value, "
                   "SUM(r.Number_of_Persons) as Total_Persons "
                   "FROM Reservations r "
                   "WHERE r.Status IN ('confirmed', 'paid')";
    
    if (!start_date.isEmpty())
    {
        query += QString(" AND r.Reservation_Date >= '%1'").arg(escape_string(start_date));
    }
    
    if (!end_date.isEmpty())
    {
        query += QString(" AND r.Reservation_Date <= '%1'").arg(escape_string(end_date));
    }
    
    return execute_select(query);
}

Query_Result Database_Manager::get_user_statistics()
{
    QString query = "SELECT COUNT(*) as Total_Users, "
                   "COUNT(CASE WHEN Date_Created >= DATEADD(month, -1, GETDATE()) THEN 1 END) as New_Users_This_Month, "
                   "COUNT(CASE WHEN Date_Created >= DATEADD(week, -1, GETDATE()) THEN 1 END) as New_Users_This_Week "
                   "FROM Users";
    return execute_select(query);
}

Query_Result Database_Manager::get_booking_statistics()
{
    QString query = "SELECT COUNT(*) as Total_Bookings, "
                   "COUNT(CASE WHEN Status = 'pending' THEN 1 END) as Pending_Bookings, "
                   "COUNT(CASE WHEN Status = 'confirmed' THEN 1 END) as Confirmed_Bookings, "
                   "COUNT(CASE WHEN Status = 'paid' THEN 1 END) as Paid_Bookings, "
                   "COUNT(CASE WHEN Status = 'cancelled' THEN 1 END) as Cancelled_Bookings, "
                   "COUNT(CASE WHEN Reservation_Date >= DATEADD(month, -1, GETDATE()) THEN 1 END) as Bookings_This_Month "
                   "FROM Reservations";
    return execute_select(query);
}

// Private helpers
bool Database_Manager::retry_operation(std::function<bool()> operation, int max_attempts)
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        if (operation())
        {
            return true;
        }
        
        if (attempt < max_attempts)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        }
    }
    
    return false;
}

// Table creation SQL
QString Database_Manager::get_create_users_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Users') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Users (
                User_ID INT PRIMARY KEY IDENTITY(1,1),
                Username VARCHAR(50) NOT NULL UNIQUE,
                Password_Hash VARCHAR(255) NOT NULL,
                Password_Salt VARCHAR(255) NOT NULL,
                Email VARCHAR(100) UNIQUE,
                First_Name VARCHAR(50),
                Last_Name VARCHAR(50),
                Phone VARCHAR(15),
                Date_Created DATETIME DEFAULT GETDATE(),
                Date_Modified DATETIME DEFAULT GETDATE()
            )
        END
    )";
}

QString Database_Manager::get_create_destinations_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Destinations') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Destinations (
                Destination_ID INT PRIMARY KEY IDENTITY(1,1),
                Name VARCHAR(100) NOT NULL,
                Country VARCHAR(100) NOT NULL,
                Description TEXT,
                Image_Path VARCHAR(255),
                Date_Created DATETIME DEFAULT GETDATE(),
                Date_Modified DATETIME DEFAULT GETDATE()
            )
        END
    )";
}

QString Database_Manager::get_create_transport_types_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Types_of_Transport') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Types_of_Transport (
                Transport_Type_ID INT PRIMARY KEY IDENTITY(1,1),
                Name VARCHAR(100) NOT NULL,
                Description TEXT,
                Date_Created DATETIME DEFAULT GETDATE(),
                Date_Modified DATETIME DEFAULT GETDATE()
            )
        END
    )";
}

QString Database_Manager::get_create_accommodation_types_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Types_of_Accommodation') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Types_of_Accommodation (
                Accommodation_Type_ID INT PRIMARY KEY IDENTITY(1,1),
                Name VARCHAR(100) NOT NULL,
                Description TEXT,
                Date_Created DATETIME DEFAULT GETDATE(),
                Date_Modified DATETIME DEFAULT GETDATE()
            )
        END
    )";
}

QString Database_Manager::get_create_accommodations_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Accommodations') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Accommodations (
                Accommodation_ID INT PRIMARY KEY IDENTITY(1,1),
                Name VARCHAR(100) NOT NULL,
                Destination_ID INT NOT NULL,
                Type_of_Accommodation INT NOT NULL,
                Category VARCHAR(10),
                Address VARCHAR(255),
                Facilities TEXT,
                Rating DECIMAL(4, 2) CHECK (Rating >= 0 AND Rating <= 10),
                Description TEXT,
                Date_Created DATETIME DEFAULT GETDATE(),
                Date_Modified DATETIME DEFAULT GETDATE(),
                FOREIGN KEY (Destination_ID) REFERENCES dbo.Destinations(Destination_ID),
                FOREIGN KEY (Type_of_Accommodation) REFERENCES dbo.Types_of_Accommodation(Accommodation_Type_ID)
            )
        END
    )";
}

QString Database_Manager::get_create_offers_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Offers') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Offers (
                Offer_ID INT PRIMARY KEY IDENTITY(1,1),
                Name VARCHAR(150) NOT NULL,
                Destination_ID INT NOT NULL,
                Accommodation_ID INT NOT NULL,
                Types_of_Transport_ID INT NOT NULL,
                Price_per_Person DECIMAL(10, 2) NOT NULL,
                Duration_Days INT NOT NULL,
                Departure_Date DATE NOT NULL,
                Return_Date DATE NOT NULL,
                Total_Seats INT NOT NULL,
                Reserved_Seats INT NOT NULL DEFAULT 0,
                Included_Services TEXT,
                Description TEXT,
                Status VARCHAR(20) NOT NULL DEFAULT 'active',
                Date_Created DATETIME DEFAULT GETDATE(),
                Date_Modified DATETIME DEFAULT GETDATE(),
                FOREIGN KEY (Destination_ID) REFERENCES dbo.Destinations(Destination_ID),
                FOREIGN KEY (Accommodation_ID) REFERENCES dbo.Accommodations(Accommodation_ID),
                FOREIGN KEY (Types_of_Transport_ID) REFERENCES dbo.Types_of_Transport(Transport_Type_ID),
                CHECK (Status IN ('active', 'inactive', 'expired'))
            )
        END
    )";
}

QString Database_Manager::get_create_reservations_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Reservations') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Reservations (
                Reservation_ID INT PRIMARY KEY IDENTITY(1,1),
                User_ID INT NOT NULL,
                Offer_ID INT NOT NULL,
                Number_of_Persons INT NOT NULL,
                Total_Price DECIMAL(10,2) NOT NULL,
                Reservation_Date DATETIME DEFAULT GETDATE(),
                Status VARCHAR(20) NOT NULL,
                Notes TEXT,
                FOREIGN KEY (User_ID) REFERENCES dbo.Users(User_ID),
                FOREIGN KEY (Offer_ID) REFERENCES dbo.Offers(Offer_ID),
                CHECK (Status IN ('pending', 'confirmed', 'paid', 'cancelled'))
            )
        END
    )";
}

QString Database_Manager::get_create_reservation_persons_table_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'dbo.Reservation_Persons') AND type = 'U')
        BEGIN
            CREATE TABLE dbo.Reservation_Persons (
                Reservation_Person_ID INT PRIMARY KEY IDENTITY(1,1),
                Reservation_ID INT NOT NULL,
                Full_Name VARCHAR(100) NOT NULL,
                CNP VARCHAR(15) NOT NULL,
                Birth_Date DATE NOT NULL,
                Person_Type VARCHAR(20) NOT NULL,
                FOREIGN KEY (Reservation_ID) REFERENCES dbo.Reservations(Reservation_ID)
            )
        END
    )";
}

QString Database_Manager::get_create_indexes_sql()
{
    return R"(
        IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_Users_Username')
            CREATE INDEX IX_Users_Username ON Users(Username);
        
        IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_Offers_Destination')
            CREATE INDEX IX_Offers_Destination ON Offers(Destination_ID);
        
        IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_Offers_Price')
            CREATE INDEX IX_Offers_Price ON Offers(Price_per_Person);
        
        IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_Offers_Status')
            CREATE INDEX IX_Offers_Status ON Offers(Status);
        
        IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_Reservations_User')
            CREATE INDEX IX_Reservations_User ON Reservations(User_ID);
        
        IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'IX_Offers_Destination_Price')
            CREATE INDEX IX_Offers_Destination_Price ON Offers(Destination_ID, Price_per_Person);
    )";
}