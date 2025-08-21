#pragma once

#include <QtCore/QString>

/**
 * User data structure matching the Users database table
 * Database fields: User_ID, Username, Password_Hash, Password_Salt, Email, 
 *                  First_Name, Last_Name, Phone, Date_Created, Date_Modified
 */
struct User_Data
{
	int id = 0;                    // User_ID
	QString username;              // Username
	QString password_hash;         // Password_Hash
	QString password_salt;         // Password_Salt
	QString email;                 // Email
	QString first_name;            // First_Name
	QString last_name;             // Last_Name
	QString phone_number;          // Phone
	QString created_at;            // Date_Created
	QString updated_at;            // Date_Modified
	
	User_Data() : id(0) 
	{
	}
	
	// Clear all data
	void clear()
	{
		id = 0;
		username.clear();
		password_hash.clear();
		password_salt.clear();
		email.clear();
		first_name.clear();
		last_name.clear();
		phone_number.clear();
		created_at.clear();
		updated_at.clear();
	}
};