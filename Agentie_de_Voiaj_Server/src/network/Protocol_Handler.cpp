#include "network/Protocol_Handler.h"
#include "network/Client_Handler.h"
#include "utils/utils.h"
#include "config.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonParseError>
#include <QtCore/QDebug>

using namespace SocketNetwork;

Protocol_Handler::Protocol_Handler(std::shared_ptr<Database::Database_Manager> db_manager)
    : db_manager(db_manager)
{
}

Parsed_Message Protocol_Handler::parse_message(const QString& json_message)
{
    Parsed_Message parsed;
    parsed.raw_message = json_message;
    
    if (json_message.isEmpty()) {
        parsed.error_message = Config::ErrorMessages::INVALID_REQUEST;
        return parsed;
    }
    
    try {
        QJsonParseError parse_error;
        QJsonDocument doc = QJsonDocument::fromJson(json_message.toUtf8(), &parse_error);
        
        if (parse_error.error != QJsonParseError::NoError) {
            parsed.error_message = "JSON parse error: " + parse_error.errorString();
            return parsed;
        }
        
        if (!doc.isObject()) {
            parsed.error_message = "JSON root must be an object";
            return parsed;
        }
        
        parsed.json_data = doc.object();
        
        // Extract command/type from JSON
        if (!parsed.json_data.contains("type") && !parsed.json_data.contains("command")) {
            parsed.error_message = "Missing 'type' or 'command' field in JSON message";
            return parsed;
        }
        
        parsed.type = get_message_type(parsed.json_data);
        
        if (parsed.type == Message_Type::UNKNOWN) {
            QString command = parsed.json_data.contains("type") ? 
                parsed.json_data["type"].toString() : 
                parsed.json_data["command"].toString();
            parsed.error_message = "Unknown command: " + command;
            return parsed;
        }
        
        parsed.is_valid = true;
        return parsed;
    }
    catch (const std::exception& e) {
        parsed.error_message = "Parse error: " + QString::fromStdString(e.what());
        return parsed;
    }
}

Message_Type Protocol_Handler::get_message_type(const QJsonObject& json_obj)
{
    try {
        QString command;
        if (json_obj.contains("type")) {
            command = json_obj["type"].toString();
        }
        else if (json_obj.contains("command")) {
            command = json_obj["command"].toString();
        }
        else {
            return Message_Type::UNKNOWN;
        }
        
        // Convert to uppercase for comparison
        QString cmd = Utils::String::to_upper(Utils::String::trim(command));
        
        if (cmd == "AUTH" || cmd == "LOGIN") return Message_Type::AUTHENTICATION;
        if (cmd == "REGISTER" || cmd == "SIGNUP") return Message_Type::REGISTRATION;
        if (cmd == "GET_DESTINATIONS") return Message_Type::GET_DESTINATIONS;
        if (cmd == "GET_OFFERS") return Message_Type::GET_OFFERS;
        if (cmd == "SEARCH_OFFERS") return Message_Type::SEARCH_OFFERS;
        if (cmd == "BOOK_OFFER") return Message_Type::BOOK_OFFER;
        if (cmd == "GET_USER_RESERVATIONS") return Message_Type::GET_USER_RESERVATIONS;
        if (cmd == "CANCEL_RESERVATION") return Message_Type::CANCEL_RESERVATION;
        if (cmd == "GET_USER_INFO") return Message_Type::GET_USER_INFO;
        if (cmd == "UPDATE_USER_INFO") return Message_Type::UPDATE_USER_INFO;
        if (cmd == "KEEPALIVE" || cmd == "PING") return Message_Type::KEEPALIVE;
        if (cmd == "ERROR") return Message_Type::ERR;
        
        return Message_Type::UNKNOWN;
    }
    catch (const std::exception&) {
        return Message_Type::UNKNOWN;
    }
}

QString Protocol_Handler::message_type_to_string(Message_Type type)
{
    switch (type) {
        case Message_Type::AUTHENTICATION: return "AUTHENTICATION";
        case Message_Type::REGISTRATION: return "REGISTRATION";
        case Message_Type::GET_DESTINATIONS: return "GET_DESTINATIONS";
        case Message_Type::GET_OFFERS: return "GET_OFFERS";
        case Message_Type::SEARCH_OFFERS: return "SEARCH_OFFERS";
        case Message_Type::BOOK_OFFER: return "BOOK_OFFER";
        case Message_Type::GET_USER_RESERVATIONS: return "GET_USER_RESERVATIONS";
        case Message_Type::CANCEL_RESERVATION: return "CANCEL_RESERVATION";
        case Message_Type::GET_USER_INFO: return "GET_USER_INFO";
        case Message_Type::UPDATE_USER_INFO: return "UPDATE_USER_INFO";
        case Message_Type::KEEPALIVE: return "KEEPALIVE";
        case Message_Type::ERR: return "ERROR";
        case Message_Type::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

Response Protocol_Handler::process_message(const Parsed_Message& parsed_message, Client_Handler* client_handler)
{
    if (!client_handler) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR);
    }
    
    try {
        switch (parsed_message.type) {
            case Message_Type::AUTHENTICATION:
                return handle_authentication(parsed_message, client_handler);
            
            case Message_Type::REGISTRATION:
                return handle_registration(parsed_message, client_handler);
            
            case Message_Type::GET_DESTINATIONS:
                return handle_get_destinations(parsed_message, client_handler);
            
            case Message_Type::GET_OFFERS:
                return handle_get_offers(parsed_message, client_handler);
            
            case Message_Type::SEARCH_OFFERS:
                return handle_search_offers(parsed_message, client_handler);
            
            case Message_Type::BOOK_OFFER:
                return handle_book_offer(parsed_message, client_handler);
            
            case Message_Type::GET_USER_RESERVATIONS:
                return handle_get_user_reservations(parsed_message, client_handler);
            
            case Message_Type::CANCEL_RESERVATION:
                return handle_cancel_reservation(parsed_message, client_handler);
            
            case Message_Type::GET_USER_INFO:
                return handle_get_user_info(parsed_message, client_handler);
            
            case Message_Type::UPDATE_USER_INFO:
                return handle_update_user_info(parsed_message, client_handler);
            
            case Message_Type::KEEPALIVE:
                return handle_keepalive(parsed_message, client_handler);
            
            default:
                return Response(false, "Unsupported message type");
        }
    }
    catch (const Utils::Exceptions::DatabaseException& e) {
        Utils::Logger::error("Database error: " + QString::fromStdString(e.what()));
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED, "", e.error_code());
    }
    catch (const Utils::Exceptions::NetworkException& e) {
        Utils::Logger::error("Network error: " + QString::fromStdString(e.what()));
        return Response(false, Config::ErrorMessages::SOCKET_COMM_ERROR, "", e.error_code());
    }
    catch (const Utils::Exceptions::ValidationException& e) {
        Utils::Logger::warning("Validation error: " + QString::fromStdString(e.what()));
        return Response(false, e.message(), "", e.error_code());
    }
    catch (const std::exception& e) {
        Utils::Logger::critical("Unexpected error: " + QString::fromStdString(e.what()));
        return Response(false, Config::ErrorMessages::SERVER_ERROR);
    }
}

QString Protocol_Handler::create_response(bool success, const QString& message,
    const QJsonValue& data, int error_code)
{
    QJsonObject response;
    response["success"] = success;
    response["message"] = message;
    
    if (success) {
        if (data.isObject() || data.isArray()) {
            response["data"] = data;
        } else if (!data.toString().isEmpty()) {
            // Try to parse as JSON string
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(data.toString().toUtf8(), &parse_error);
            if (parse_error.error == QJsonParseError::NoError) {
                if (doc.isObject()) {
                    response["data"] = doc.object();
                } else if (doc.isArray()) {
                    response["data"] = doc.array();
                } else {
                    response["data"] = data.toString();
                }
            } else {
                response["data"] = data.toString();
            }
        } else {
            response["data"] = QJsonObject();
        }
    }
    
    if (!success && error_code != 0) {
        response["error_code"] = error_code;
    }
    
    QJsonDocument doc(response);
    return doc.toJson(QJsonDocument::Compact);
}

Response Protocol_Handler::handle_authentication(const Parsed_Message& message, Client_Handler* client)
{
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    // Check required fields in JSON
    if (!message.json_data.contains("username") || !message.json_data.contains("password")) {
        return Response(false, "Missing required fields: username, password");
    }
    
    try {
        QString username = message.json_data["username"].toString();
        QString password = message.json_data["password"].toString();
        
        Utils::Logger::info("Authentication attempt for user: " + username + " from IP: " + client->get_client_info().ip_address);
        
        auto result = db_manager->authenticate_user(username, password);
        
        if (result.is_success() && result.has_data()) {
            int user_id = result.data[0]["ID"].toInt();
            client->set_authenticated(user_id, username);
            
            Utils::Logger::info("Authentication SUCCESS: User '" + username + "' (ID:" + QString::number(user_id) + ") logged in from " + client->get_client_info().ip_address);
            
            QJsonObject user_data;
            for (auto it = result.data[0].constBegin(); it != result.data[0].constEnd(); ++it) {
                user_data[it.key()] = QJsonValue::fromVariant(it.value());
            }
            
            QJsonDocument doc(user_data);
            return Response(true, Config::SuccessMessages::LOGIN_SUCCESS, doc.toJson(QJsonDocument::Compact));
        }
        else {
            Utils::Logger::warning("Authentication FAILED: Invalid credentials for user '" + username + "' from " + client->get_client_info().ip_address);
            return Response(false, Config::ErrorMessages::AUTHENTICATION_FAILED);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_registration(const Parsed_Message& message, Client_Handler* client)
{
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    // Check required fields in JSON
    const QStringList required_fields = {"username", "password", "email", "first_name", "last_name"};
    for (const QString& field : required_fields) {
        if (!message.json_data.contains(field)) {
            return Response(false, "Missing required field: " + field);
        }
    }
    
    try {
        User_Data user_data;
        user_data.username = message.json_data["username"].toString();
        user_data.password_hash = message.json_data["password"].toString(); // Will be hashed in DB layer
        user_data.email = message.json_data["email"].toString();
        user_data.first_name = message.json_data["first_name"].toString();
        user_data.last_name = message.json_data["last_name"].toString();
        
        Utils::Logger::info("Registration attempt for user: " + user_data.username + " (" + user_data.email + ") from IP: " + client->get_client_info().ip_address);
        
        if (message.json_data.contains("phone_number")) {
            user_data.phone_number = message.json_data["phone_number"].toString();
        }
        
        auto result = db_manager->register_user(user_data);
        
        if (result.is_success()) {
            Utils::Logger::info("Registration SUCCESS: New user '" + user_data.username + "' created successfully from " + client->get_client_info().ip_address);
            return Response(true, Config::SuccessMessages::USER_CREATED);
        }
        else {
            Utils::Logger::warning("Registration FAILED: " + result.message + " for user '" + user_data.username + "' from " + client->get_client_info().ip_address);
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_get_destinations(const Parsed_Message& message, Client_Handler* client)
{
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    try {
        // Check if we're in demo mode and use mock data
        auto result = db_manager->is_running_in_demo_mode() ? 
            db_manager->create_mock_response("get_destinations") : 
            db_manager->get_all_destinations();
        
        if (result.is_success()) {
            QJsonArray destinations_json = vector_to_json(result.data);
            QString message_text = db_manager->is_running_in_demo_mode() ? 
                "Demo destinations retrieved successfully" : 
                Config::SuccessMessages::DATA_RETRIEVED;
            
            QJsonDocument doc(destinations_json);
            return Response(true, message_text, doc.toJson(QJsonDocument::Compact));
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_get_offers(const Parsed_Message& message, Client_Handler* client)
{
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    try {
        // Check if we're in demo mode and use mock data
        auto result = db_manager->is_running_in_demo_mode() ? 
            db_manager->create_mock_response("get_offers") : 
            db_manager->get_available_offers();
        
        if (result.is_success()) {
            QJsonArray offers_json = vector_to_json(result.data);
            QString message_text = db_manager->is_running_in_demo_mode() ? 
                "Demo offers retrieved successfully" : 
                Config::SuccessMessages::DATA_RETRIEVED;
            
            QJsonDocument doc(offers_json);
            return Response(true, message_text, doc.toJson(QJsonDocument::Compact));
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_search_offers(const Parsed_Message& message, Client_Handler* client)
{
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    try {
        QString destination = message.json_data.contains("destination") ? message.json_data["destination"].toString() : "";
        qreal min_price = message.json_data.contains("min_price") ? message.json_data["min_price"].toDouble() : 0.0;
        qreal max_price = message.json_data.contains("max_price") ? message.json_data["max_price"].toDouble() : 0.0;
        QString start_date = message.json_data.contains("start_date") ? message.json_data["start_date"].toString() : "";
        QString end_date = message.json_data.contains("end_date") ? message.json_data["end_date"].toString() : "";
        
        auto result = db_manager->search_offers(destination, min_price, max_price, start_date, end_date);
        
        if (result.is_success()) {
            QJsonArray offers_json = vector_to_json(result.data);
            QJsonDocument doc(offers_json);
            return Response(true, Config::SuccessMessages::DATA_RETRIEVED, doc.toJson(QJsonDocument::Compact));
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_book_offer(const Parsed_Message& message, Client_Handler* client)
{
    if (!client->is_authenticated()) {
        return Response(false, Config::ErrorMessages::AUTHENTICATION_FAILED);
    }
    
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    if (!message.json_data.contains("offer_id")) {
        return Response(false, "Missing required field: offer_id");
    }
    
    try {
        int offer_id = message.json_data["offer_id"].toInt();
        int person_count = message.json_data.contains("person_count") ? message.json_data["person_count"].toInt() : 1;
        
        // Validate person count
        if (person_count < 1 || person_count > Config::Business::MAX_PERSONS_PER_RESERVATION) {
            return Response(false, "Invalid person count");
        }
        
        auto result = db_manager->book_offer(client->get_client_info().user_id, offer_id, person_count);
        
        if (result.is_success()) {
            return Response(true, Config::SuccessMessages::RESERVATION_CREATED);
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_get_user_reservations(const Parsed_Message& message, Client_Handler* client)
{
    if (!client->is_authenticated()) {
        return Response(false, Config::ErrorMessages::AUTHENTICATION_FAILED);
    }
    
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    try {
        auto result = db_manager->get_user_reservations(client->get_client_info().user_id);
        
        if (result.is_success()) {
            QJsonArray reservations_json = vector_to_json(result.data);
            QJsonDocument doc(reservations_json);
            return Response(true, Config::SuccessMessages::DATA_RETRIEVED, doc.toJson(QJsonDocument::Compact));
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_cancel_reservation(const Parsed_Message& message, Client_Handler* client)
{
    if (!client->is_authenticated()) {
        return Response(false, Config::ErrorMessages::AUTHENTICATION_FAILED);
    }
    
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    if (!message.json_data.contains("reservation_id")) {
        return Response(false, "Missing required field: reservation_id");
    }
    
    try {
        int reservation_id = message.json_data["reservation_id"].toInt();
        
        auto result = db_manager->cancel_reservation(reservation_id);
        
        if (result.is_success()) {
            return Response(true, Config::SuccessMessages::RESERVATION_CANCELLED);
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_get_user_info(const Parsed_Message& message, Client_Handler* client)
{
    if (!client->is_authenticated()) {
        return Response(false, Config::ErrorMessages::AUTHENTICATION_FAILED);
    }
    
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    try {
        auto result = db_manager->get_user_by_id(client->get_client_info().user_id);
        
        if (result.is_success() && result.has_data()) {
            QJsonObject user_data;
            for (auto it = result.data[0].constBegin(); it != result.data[0].constEnd(); ++it) {
                user_data[it.key()] = QJsonValue::fromVariant(it.value());
            }
            QJsonDocument doc(user_data);
            return Response(true, Config::SuccessMessages::DATA_RETRIEVED, doc.toJson(QJsonDocument::Compact));
        }
        else {
            return Response(false, Config::ErrorMessages::USER_NOT_FOUND);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_update_user_info(const Parsed_Message& message, Client_Handler* client)
{
    if (!client->is_authenticated()) {
        return Response(false, Config::ErrorMessages::AUTHENTICATION_FAILED);
    }
    
    if (!db_manager) {
        return Response(false, Config::ErrorMessages::DB_CONNECTION_FAILED);
    }
    
    try {
        // Get current user data
        auto current_result = db_manager->get_user_by_id(client->get_client_info().user_id);
        if (!current_result.is_success() || !current_result.has_data()) {
            return Response(false, Config::ErrorMessages::USER_NOT_FOUND);
        }
        
        User_Data user_data;
        user_data.id = client->get_client_info().user_id;
        user_data.username = current_result.data[0]["Username"].toString();
        user_data.password_hash = current_result.data[0]["Password_Hash"].toString();
        
        // Update only provided fields
        user_data.email = message.json_data.contains("email") ? 
                          message.json_data["email"].toString() : current_result.data[0]["Email"].toString();
        user_data.first_name = message.json_data.contains("first_name") ? 
                              message.json_data["first_name"].toString() : current_result.data[0]["First_Name"].toString();
        user_data.last_name = message.json_data.contains("last_name") ? 
                             message.json_data["last_name"].toString() : current_result.data[0]["Last_Name"].toString();
        user_data.phone_number = message.json_data.contains("phone_number") ? 
                                message.json_data["phone_number"].toString() : current_result.data[0]["Phone"].toString();
        
        auto result = db_manager->update_user(user_data);
        
        if (result.is_success()) {
            return Response(true, "User information updated successfully");
        }
        else {
            return Response(false, result.message);
        }
    }
    catch (const std::exception& e) {
        return Response(false, Config::ErrorMessages::SERVER_ERROR + ": " + QString::fromStdString(e.what()));
    }
}

Response Protocol_Handler::handle_keepalive(const Parsed_Message& /*message*/, Client_Handler* /*client*/)
{
    return Response(true, "PONG");
}

bool Protocol_Handler::is_user_admin(int /*user_id*/)
{
    // Not implemented in college project scope - all users are regular clients
    return false;
}

QJsonArray Protocol_Handler::vector_to_json(const QList<QHash<QString, QVariant>>& data)
{
    QJsonArray json_array;
    for (const auto& row : data) {
        QJsonObject json_obj;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            json_obj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        json_array.append(json_obj);
    }
    
    return json_array;
}