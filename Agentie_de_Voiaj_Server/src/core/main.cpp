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

// Original server main function - commented out for database testing

int main(int argc, char *argv[])
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


// NEW: Complete database testing main function
//int main(int argc, char *argv[])
//{
//    QCoreApplication app(argc, argv);
//
//    qDebug() << "=== COMPLETE DATABASE TESTING ===";
//    
//    // Create database manager instance
//    Database::Database_Manager db_manager;
//    
//    // Set connection parameters for SQL Server external database
//    QString server = "DESKTOP-tibosul\\SQLEXPRESS"; // Your SQL Server instance name
//    QString database = "Agentie_de_Voiaj"; // Your database name
//    QString username = ""; // leave empty for Windows Authentication 
//    QString password = ""; // leave empty for Windows Authentication
//    
//    qDebug() << "Attempting to connect to database...";
//    qDebug() << "Server:" << server;
//    qDebug() << "Database:" << database;
//    
//    // Try to connect
//    if (db_manager.connect(server, database, username, password))
//    {
//        qDebug() << "✓ Database connection successful!";
//        
//        // Test all main tables
//        qDebug() << "\n" << QString(60, '=');
//        qDebug() << "TESTING ALL DATABASE TABLES";
//        qDebug() << QString(60, '=');
//        
//        // 1. Users Table
//        qDebug() << "\n=== 1. USERS TABLE ===";
//        Database::Query_Result users_result = db_manager.execute_select("SELECT * FROM Users");
//        if (users_result.is_success() && users_result.has_data()) {
//            qDebug() << QString("Found %1 users:").arg(users_result.data.size());
//            for (const auto& user : users_result.data) {
//                qDebug() << QString("  - ID: %1, Username: %2, Email: %3")
//                    .arg(user["User_ID"].toInt())
//                    .arg(user["Username"].toString())
//                    .arg(user["Email"].toString());
//            }
//        } else {
//            qDebug() << "Users table is empty or error:" << users_result.message;
//        }
//        
//        // 2. Destinations Table
//        qDebug() << "\n=== 2. DESTINATIONS TABLE ===";
//        Database::Query_Result dest_result = db_manager.execute_select("SELECT * FROM Destinations");
//        if (dest_result.is_success() && dest_result.has_data()) {
//            qDebug() << QString("Found %1 destinations:").arg(dest_result.data.size());
//            for (const auto& dest : dest_result.data) {
//                qDebug() << QString("  - ID: %1, Name: %2, Country: %3")
//                    .arg(dest["Destination_ID"].toInt())
//                    .arg(dest["Name"].toString())
//                    .arg(dest["Country"].toString());
//            }
//        } else {
//            qDebug() << "Destinations table is empty or error:" << dest_result.message;
//        }
//        
//        // 3. Types_of_Transport Table
//        qDebug() << "\n=== 3. TYPES_OF_TRANSPORT TABLE ===";
//        Database::Query_Result transport_result = db_manager.execute_select("SELECT * FROM Types_of_Transport");
//        if (transport_result.is_success() && transport_result.has_data()) {
//            qDebug() << QString("Found %1 transport types:").arg(transport_result.data.size());
//            for (const auto& transport : transport_result.data) {
//                qDebug() << QString("  - ID: %1, Name: %2")
//                    .arg(transport["Transport_Type_ID"].toInt())
//                    .arg(transport["Name"].toString());
//            }
//        } else {
//            qDebug() << "Types_of_Transport table is empty or error:" << transport_result.message;
//        }
//        
//        // 4. Types_of_Accommodation Table
//        qDebug() << "\n=== 4. TYPES_OF_ACCOMMODATION TABLE ===";
//        Database::Query_Result accom_types_result = db_manager.execute_select("SELECT * FROM Types_of_Accommodation");
//        if (accom_types_result.is_success() && accom_types_result.has_data()) {
//            qDebug() << QString("Found %1 accommodation types:").arg(accom_types_result.data.size());
//            for (const auto& accom_type : accom_types_result.data) {
//                qDebug() << QString("  - ID: %1, Name: %2")
//                    .arg(accom_type["Accommodation_Type_ID"].toInt())
//                    .arg(accom_type["Name"].toString());
//            }
//        } else {
//            qDebug() << "Types_of_Accommodation table is empty or error:" << accom_types_result.message;
//        }
//        
//        // 5. Accommodations Table
//        qDebug() << "\n=== 5. ACCOMMODATIONS TABLE ===";
//        Database::Query_Result accom_result = db_manager.execute_select("SELECT * FROM Accommodations");
//        if (accom_result.is_success() && accom_result.has_data()) {
//            qDebug() << QString("Found %1 accommodations:").arg(accom_result.data.size());
//            for (const auto& accom : accom_result.data) {
//                qDebug() << QString("  - ID: %1, Name: %2, Dest_ID: %3, Category: %4")
//                    .arg(accom["Accommodation_ID"].toInt())
//                    .arg(accom["Name"].toString())
//                    .arg(accom["Destination_ID"].toInt())
//                    .arg(accom["Category"].toString());
//            }
//        } else {
//            qDebug() << "Accommodations table is empty or error:" << accom_result.message;
//        }
//        
//        // 6. Offers Table (RAW)
//        qDebug() << "\n=== 6. OFFERS TABLE (RAW) ===";
//        Database::Query_Result offers_raw = db_manager.execute_select("SELECT * FROM Offers");
//        if (offers_raw.is_success() && offers_raw.has_data()) {
//            qDebug() << QString("Found %1 raw offers:").arg(offers_raw.data.size());
//            for (const auto& offer : offers_raw.data) {
//                qDebug() << QString("  - ID: %1, Name: %2, Dest_ID: %3, Price: %4, Status: %5")
//                    .arg(offer["Offer_ID"].toInt())
//                    .arg(offer["Name"].toString())
//                    .arg(offer["Destination_ID"].toInt())
//                    .arg(offer["Price_per_Person"].toDouble())
//                    .arg(offer["Status"].toString());
//            }
//        } else {
//            qDebug() << "Offers table is empty or error:" << offers_raw.message;
//        }
//        
//        // 7. Reservations Table
//        qDebug() << "\n=== 7. RESERVATIONS TABLE ===";
//        Database::Query_Result reserv_result = db_manager.execute_select("SELECT * FROM Reservations");
//        if (reserv_result.is_success() && reserv_result.has_data()) {
//            qDebug() << QString("Found %1 reservations:").arg(reserv_result.data.size());
//            for (const auto& reserv : reserv_result.data) {
//                qDebug() << QString("  - ID: %1, User_ID: %2, Offer_ID: %3, Persons: %4, Price: %5, Status: %6")
//                    .arg(reserv["Reservation_ID"].toInt())
//                    .arg(reserv["User_ID"].toInt())
//                    .arg(reserv["Offer_ID"].toInt())
//                    .arg(reserv["Number_of_Persons"].toInt())
//                    .arg(reserv["Total_Price"].toDouble())
//                    .arg(reserv["Status"].toString());
//            }
//        } else {
//            qDebug() << "Reservations table is empty or error:" << reserv_result.message;
//        }
//        
//        // Now test the VIEWS and Database_Manager methods
//        qDebug() << "\n" << QString(60, '=');
//        qDebug() << "TESTING VIEWS AND DATABASE_MANAGER METHODS";
//        qDebug() << QString(60, '=');
//        
//        // 8. Test get_available_offers() method (what client actually calls)
//        qDebug() << "\n=== 8. DATABASE_MANAGER::get_available_offers() ===";
//        Database::Query_Result available_offers = db_manager.get_available_offers();
//        if (available_offers.is_success() && available_offers.has_data()) {
//            qDebug() << QString("✓ Found %1 available offers:").arg(available_offers.data.size());
//            for (const auto& offer : available_offers.data) {
//                qDebug() << QString("  - ID: %1, Name: %2, Dest: %3, Accom: %4, Transport: %5, Price: %6")
//                    .arg(offer["Offer_ID"].toInt())
//                    .arg(offer["Name"].toString())
//                    .arg(offer["Destination_Name"].toString())
//                    .arg(offer["Accommodation_Name"].toString())
//                    .arg(offer["Transport_Name"].toString())
//                    .arg(offer["Price_per_Person"].toDouble());
//                qDebug() << QString("    Seats: Total=%1, Reserved=%2, Available=%3")
//                    .arg(offer["Total_Seats"].toInt())
//                    .arg(offer["Reserved_Seats"].toInt())
//                    .arg(offer["Total_Seats"].toInt() - offer["Reserved_Seats"].toInt());
//            }
//        } else {
//            qDebug() << "✗ get_available_offers() failed:" << available_offers.message;
//        }
//        
//        // 9. Test get_all_destinations() method
//        qDebug() << "\n=== 9. DATABASE_MANAGER::get_all_destinations() ===";
//        Database::Query_Result all_destinations = db_manager.get_all_destinations();
//        if (all_destinations.is_success() && all_destinations.has_data()) {
//            qDebug() << QString("✓ Found %1 destinations via get_all_destinations():").arg(all_destinations.data.size());
//            for (const auto& dest : all_destinations.data) {
//                qDebug() << QString("  - ID: %1, Name: %2, Country: %3")
//                    .arg(dest["Destination_ID"].toInt())
//                    .arg(dest["Name"].toString())
//                    .arg(dest["Country"].toString());
//            }
//        } else {
//            qDebug() << "✗ get_all_destinations() failed:" << all_destinations.message;
//        }
//        
//        // 10. Test Views directly
//        qDebug() << "\n=== 10. VIEW: v_Offers_Complete ===";
//        Database::Query_Result view_offers = db_manager.execute_select("SELECT * FROM v_Offers_Complete");
//        if (view_offers.is_success() && view_offers.has_data()) {
//            qDebug() << QString("✓ Found %1 offers in v_Offers_Complete view:").arg(view_offers.data.size());
//            for (const auto& offer : view_offers.data) {
//                qDebug() << QString("  - ID: %1, Name: %2, Dest: %3, Price: %4")
//                    .arg(offer["Offer_ID"].toInt())
//                    .arg(offer["Offer_Name"].toString())
//                    .arg(offer["Destination"].toString())
//                    .arg(offer["Price_per_Person"].toDouble());
//            }
//        } else {
//            qDebug() << "✗ v_Offers_Complete view failed:" << view_offers.message;
//        }
//        
//        qDebug() << "\n=== 11. VIEW: v_User_Reservations ===";
//        Database::Query_Result view_reservations = db_manager.execute_select("SELECT * FROM v_User_Reservations");
//        if (view_reservations.is_success() && view_reservations.has_data()) {
//            qDebug() << QString("✓ Found %1 reservations in v_User_Reservations view:").arg(view_reservations.data.size());
//            for (const auto& reserv : view_reservations.data) {
//                qDebug() << QString("  - ID: %1, User: %2, Offer: %3, Dest: %4")
//                    .arg(reserv["Reservation_ID"].toInt())
//                    .arg(reserv["Username"].toString())
//                    .arg(reserv["Offer_Name"].toString())
//                    .arg(reserv["Destination"].toString());
//            }
//        } else {
//            qDebug() << "✗ v_User_Reservations view failed:" << view_reservations.message;
//        }
//        
//        // 12. DEBUGGING THE FILTER ISSUE
//        qDebug() << "\n=== 12. DEBUGGING DATE FILTER ===";
//        qDebug() << "Current date from SQL Server:";
//        Database::Query_Result current_date = db_manager.execute_select("SELECT GETDATE() as CurrentDate");
//        if (current_date.is_success() && current_date.has_data()) {
//            qDebug() << QString("  SQL Server current date: %1").arg(current_date.data[0]["CurrentDate"].toString());
//        }
//        
//        qDebug() << "\nChecking departure dates in offers:";
//        Database::Query_Result all_dates = db_manager.execute_select("SELECT Offer_ID, Name, Departure_Date, Status FROM Offers ORDER BY Departure_Date");
//        if (all_dates.is_success() && all_dates.has_data()) {
//            for (const auto& offer : all_dates.data) {
//                qDebug() << QString("  ID: %1, Name: %2, Departure: %3, Status: %4")
//                    .arg(offer["Offer_ID"].toInt())
//                    .arg(offer["Name"].toString())
//                    .arg(offer["Departure_Date"].toString())
//                    .arg(offer["Status"].toString());
//            }
//        }
//        
//        qDebug() << "\nTesting get_available_offers() query step by step:";
//        
//        // Test without date filter
//        Database::Query_Result no_date_filter = db_manager.execute_select(
//            "SELECT o.Offer_ID, o.Name, o.Departure_Date, o.Status, o.Total_Seats, o.Reserved_Seats "
//            "FROM Offers o "
//            "WHERE o.Status = 'active' AND o.Reserved_Seats < o.Total_Seats "
//            "ORDER BY o.Departure_Date"
//        );
//        qDebug() << QString("\nWithout date filter: %1 offers").arg(no_date_filter.data.size());
//        
//        // Test with date filter
//        Database::Query_Result with_date_filter = db_manager.execute_select(
//            "SELECT o.Offer_ID, o.Name, o.Departure_Date, o.Status, o.Total_Seats, o.Reserved_Seats "
//            "FROM Offers o "
//            "WHERE o.Status = 'active' AND o.Reserved_Seats < o.Total_Seats AND o.Departure_Date > GETDATE() "
//            "ORDER BY o.Departure_Date"
//        );
//        qDebug() << QString("With date filter: %1 offers").arg(with_date_filter.data.size());
//        
//        if (with_date_filter.data.size() < no_date_filter.data.size()) {
//            qDebug() << "⚠️  DATE FILTER IS REMOVING OFFERS!";
//            qDebug() << "This might be why client receives 0 offers if all departure dates are in the past.";
//        }
//        
//        db_manager.disconnect();
//        qDebug() << "\n✓ Database disconnected successfully.";
//    }
//    else
//    {
//        qDebug() << "✗ Failed to connect to database!";
//        qDebug() << "Connection string:" << db_manager.get_connection_string();
//        qDebug() << "Last error:" << db_manager.get_last_error();
//        
//        qDebug() << "\nPlease check:";
//        qDebug() << "1. SQL Server is running";
//        qDebug() << "2. SQL Server instance name is correct";
//        qDebug() << "3. Database 'Agentie_de_Voiaj' exists";
//        qDebug() << "4. Windows Authentication is enabled";
//        qDebug() << "5. SQL Server TCP/IP protocol is enabled";
//    }
//    
//    qDebug() << "\n" << QString(60, '=');
//    qDebug() << "DATABASE TEST COMPLETED";
//    qDebug() << QString(60, '=');
//    qDebug() << "Press Ctrl+C to exit...";
//    
//    return app.exec();
//}

#include "main.moc"

