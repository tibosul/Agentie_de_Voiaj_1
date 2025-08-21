#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QDebug>
#include "utils/utils.h"
#include "database/Database_Manager.h"
#include "network/Socket_Server.h"
#include "config.h"

using namespace Database;
using namespace SocketNetwork;

class ServerApplication : public QObject
{
    Q_OBJECT

private:
    Socket_Server* server;
    QTimer* stats_timer;

public slots:
    void handleShutdown()
    {
        qDebug() << "Received shutdown signal. Shutting down server...";
        if (server)
        {
            server->stop();
        }
        QCoreApplication::quit();
    }

    void printServerStats()
    {
        if (server && Config::Application::DEBUG_MODE)
        {
            auto stats = server->get_server_stats();
            Utils::Logger::info("=== Server Stats ===");
            Utils::Logger::info("Active clients: " + QString::number(stats.active_clients));
            Utils::Logger::info("Total connections: " + QString::number(stats.total_connections));
            Utils::Logger::info("Messages received: " + QString::number(stats.total_messages_received));
            Utils::Logger::info("Messages sent: " + QString::number(stats.total_messages_sent));
            Utils::Logger::info("Uptime: " + stats.uptime);
        }
    }

public:
    ServerApplication(QObject* parent = nullptr) : QObject(parent), server(nullptr), stats_timer(nullptr) {}
    
    void setServer(Socket_Server* s) 
    { 
        server = s;
        stats_timer = new QTimer(this);
        connect(stats_timer, &QTimer::timeout, this, &ServerApplication::printServerStats);
        stats_timer->start(30000); // 30 seconds
    }
};

// Original main function - commented out for database testing
/*
int main_original(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Initialize logging system first
    Utils::Logger::initialize_logging();
    
    qDebug() << "=== AGENTIE DE VOIAJ SERVER ===";
    qDebug() << "Version:" << Config::Application::VERSION;
    qDebug() << "Debug Mode:" << (Config::Application::DEBUG_MODE ? "ON" : "OFF");
    qDebug() << "Port:" << Config::Server::PORT;
    qDebug() << "Max Connections:" << Config::Server::MAX_CONNECTIONS;
    qDebug() << QString(50, '=');
    
    // Create server application for signal handling
    ServerApplication serverApp;
    
    try 
    {
        // Create database manager - try multiple server options
        Utils::Logger::info("Initializing database manager...");
        
        // More intelligent server detection - try most common configurations first
        QStringList server_options = {
            "localhost",                    // Standard localhost
            ".",                           // Local instance
            ".\\SQLEXPRESS",               // SQL Server Express (most common)
            "localhost\\SQLEXPRESS",       // SQL Server Express on localhost
            "(LocalDB)\\MSSQLLocalDB",     // SQL Server LocalDB
            "localhost\\MSSQLSERVER",      // SQL Server default instance
            ".\\MSSQLSERVER"               // SQL Server default instance local
        };
        
        std::shared_ptr<Database_Manager> db_manager = nullptr;
        bool connected = false;
        QString successful_server;
        
        for (const auto& server : server_options)
        {
            Utils::Logger::debug("Trying database server: " + server);
            db_manager = std::make_shared<Database_Manager>(
                server,
                Config::Database::DEFAULT_DATABASE,
                "", // Windows Authentication
                ""
            );
            
            if (db_manager->connect())
            {
                Utils::Logger::info("✅ Connected to database server: " + server);
                
                // Check if the database exists
                if (db_manager->database_exists())
                {
                    Utils::Logger::info("✅ Database '" + Config::Database::DEFAULT_DATABASE + "' exists and is accessible");
                    successful_server = server;
                    connected = true;
                    break;
                }
                else
                {
                    Utils::Logger::warning("⚠️  Connected to server but database '" + Config::Database::DEFAULT_DATABASE + "' doesn't exist");
                    Utils::Logger::info("💡 Run 'setup_database.bat' to create the database and tables");
                }
            }
            else
            {
                Utils::Logger::warning("❌ Database connection failed for " + server + ": " + db_manager->get_last_error());
            }
        }
        
        if (!connected)
        {
            Utils::Logger::error("Cannot connect to any SQL Server instance!");
            Utils::Logger::info("Tried servers: localhost, localhost\\SQLEXPRESS, .\\SQLEXPRESS, (LocalDB)\\MSSQLLocalDB, .");
            
            qDebug() << "\n" << QString(60, '=');
            qDebug() << "🔶 DATABASE CONNECTION FAILED - STARTING IN DEMO MODE 🔶";
            qDebug() << QString(60, '=');
            qDebug() << "\n📋 DEMO MODE FEATURES:";
            qDebug() << "  ✅ User authentication (demo/demo123, admin/admin123, test/test123)";
            qDebug() << "  ✅ User registration (mock responses)";
            qDebug() << "  ✅ View destinations (Paris, Rome)";
            qDebug() << "  ✅ View offers (Paris Weekend, Rome Adventure)";
            qDebug() << "  ✅ All server functionality for testing";
            qDebug() << "\n📢 To enable REAL database:";
            qDebug() << "  1. Install SQL Server LocalDB or Express";
            qDebug() << "  2. Run 'setup_database.bat' to create database and tables";
            qDebug() << "  3. Or manually create 'Agentie_de_Voiaj' database";
            qDebug() << "  4. Restart the server";
            qDebug() << "\n" << QString(60, '-');
            qDebug() << "Press Enter to continue in DEMO MODE or Ctrl+C to exit...";
            
            // For Qt console application, we'll proceed automatically instead of waiting for input
            
            // Create a dummy database manager that will handle errors gracefully
            db_manager = std::make_shared<Database_Manager>("dummy", "dummy", "", "");
            Utils::Logger::warning("Server starting in DEMO MODE with mock data");
        }
        if (connected)
        {
            Utils::Logger::info("Database connection successful");
            
            // Create database tables if not exist
            Utils::Logger::info("Creating database schema...");
            if (!db_manager->create_tables_if_not_exists())
            {
                Utils::Logger::warning("Could not create/verify all database tables");
            }
            else
            {
                Utils::Logger::info("Database schema ready");
            }
        }
        else
        {
            Utils::Logger::warning("Database functionality disabled - running in fallback mode");
        }
        
        // Create server configuration
        Server_Config config;
        config.ip_address = "0.0.0.0"; // Listen on all interfaces
        config.port = Config::Server::PORT;
        config.max_clients = Config::Server::MAX_CONNECTIONS;
        config.enable_logging = Config::Application::DEBUG_MODE;
        
        // Create and configure server
        Utils::Logger::info("Creating server...");
        Socket_Server server(config);
        serverApp.setServer(&server); // Set server for signal handling
        
        server.set_database_manager(db_manager);
        
        // Connect Qt signals for graceful shutdown
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &serverApp, &ServerApplication::handleShutdown);
        
        // Initialize server
        if (!server.initialize())
        {
            qDebug() << "ERROR: Server initialization failed!";
            return -1;
        }
        Utils::Logger::info("Server initialized successfully");
        
        // Start server
        Utils::Logger::info("Starting server on " + config.ip_address + ":" + QString::number(config.port) + "...");
        if (!server.start())
        {
            qDebug() << "ERROR: Server startup failed!";
            return -1;
        }
        
        Utils::Logger::info("=== SERVER STARTED SUCCESSFULLY! ===");
        if (db_manager && db_manager->is_running_in_demo_mode())
        {
            Utils::Logger::warning("⚠️  RUNNING IN DEMO MODE - Using mock data ⚠️");
        }
        Utils::Logger::info("Server is running. Press Ctrl+C to shutdown.");
        qDebug() << "\n" << QString(50, '=');
        qDebug() << "SERVER RUNNING" << (db_manager && db_manager->is_running_in_demo_mode() ? " (DEMO MODE)" : "");
        qDebug() << "Check logs/server_" << Utils::DateTime::get_current_date() << ".log for detailed logs";
        qDebug() << QString(50, '=');
        
        // Run Qt event loop instead of manual loop
        return app.exec();
        
    }
    catch (const std::exception& e)
    {
        qDebug() << "FATAL ERROR:" << e.what();
        return -1;
    }
}
*/

// Temporary main function for database testing
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== TEMPORARY MAIN FUNCTION FOR DATABASE TESTING ===";
    
    // Create database manager instance
    Database::Database_Manager db_manager;
    
    // Set connection parameters for SQL Server external database
    QString server = "DESKTOP-tibosul\\SQLEXPRESS"; // Your SQL Server instance name
    QString database = "Agentie_de_Voiaj"; // Your database name
    QString username = ""; // leave empty for Windows Authentication 
    QString password = ""; // leave empty for Windows Authentication
    
    qDebug() << "Attempting to connect to database...";
    qDebug() << "Server:" << server;
    qDebug() << "Database:" << database;
    
    // Try to connect
    if (db_manager.connect(server, database, username, password))
    {
        qDebug() << "✓ Database connection successful!";
        
        // Query Users table
        qDebug() << "\n=== USERS TABLE CONTENT ===";
        Database::Query_Result result = db_manager.execute_select("SELECT * FROM Users");
        
        if (result.is_success())
        {
            if (result.has_data())
            {
                qDebug() << QString("Found %1 users in the database:").arg(result.data.size());
                qDebug() << "----------------------------------------";
                
                for (int i = 0; i < result.data.size(); ++i)
                {
                    const auto& user = result.data[i];
                    qDebug() << QString("User %1:").arg(i + 1);
                    qDebug() << QString("  ID: %1").arg(user["User_ID"].toInt());
                    qDebug() << QString("  Username: %1").arg(user["Username"].toString());
                    qDebug() << QString("  Email: %1").arg(user["Email"].toString());
                    qDebug() << QString("  First Name: %1").arg(user["First_Name"].toString());
                    qDebug() << QString("  Last Name: %1").arg(user["Last_Name"].toString());
                    qDebug() << QString("  Phone: %1").arg(user["Phone"].toString());
                    qDebug() << QString("  Date Created: %1").arg(user["Date_Created"].toString());
                    qDebug() << "----------------------------------------";
                }
            }
            else
            {
                qDebug() << "✓ Query executed successfully, but Users table is empty.";
            }
        }
        else
        {
            qDebug() << "✗ Failed to query Users table:" << result.message;
        }
        
        // Also try to query other tables to verify database structure
        qDebug() << "\n=== CHECKING OTHER TABLES ===";
        QStringList tables = {"Destinations", "Types_of_Transport", "Types_of_Accommodation", "Offers", "Reservations"};
        
        for (const QString& table : tables)
        {
            Database::Query_Result count_result = db_manager.execute_select(QString("SELECT COUNT(*) as record_count FROM %1").arg(table));
            if (count_result.is_success() && count_result.has_data())
            {
                int count = count_result.data[0]["record_count"].toInt();
                qDebug() << QString("Table %1: %2 records").arg(table).arg(count);
            }
            else
            {
                qDebug() << QString("Table %1: Error or doesn't exist").arg(table);
            }
        }
        
        db_manager.disconnect();
        qDebug() << "\n✓ Database disconnected successfully.";
    }
    else
    {
        qDebug() << "✗ Failed to connect to database!";
        qDebug() << "Connection string:" << db_manager.get_connection_string();
        qDebug() << "Last error:" << db_manager.get_last_error();
        
        qDebug() << "\nPlease check:";
        qDebug() << "1. SQL Server is running";
        qDebug() << "2. SQL Server instance name is correct (e.g., .\\SQLEXPRESS or DESKTOP-tibosul\\SQLEXPRESS)";
        qDebug() << "3. Database 'Agentie_de_Voiaj' exists";
        qDebug() << "4. Windows Authentication is enabled OR provide SQL credentials";
        qDebug() << "5. SQL Server TCP/IP protocol is enabled";
        qDebug() << "6. Firewall allows SQL Server connections";
    }
    
    qDebug() << "\n=== DATABASE TEST COMPLETED ===";
    qDebug() << "Press Ctrl+C to exit...";
    
    return app.exec();
}

#include "main.moc"

