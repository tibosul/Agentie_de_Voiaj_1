#pragma once

#include <QtCore/QString>

/**
 * Transport type data structure matching the Types_of_Transport database table
 * Database fields: Transport_Type_ID, Name, Description, Date_Created, Date_Modified
 */
struct Transport_Type_Data
{
	int id = 0;                    // Transport_Type_ID
	QString name;              // Name
	QString description;       // Description
	QString created_at;        // Date_Created
	QString updated_at;        // Date_Modified
	
	Transport_Type_Data() : id(0) 
	{
	}
	
	// Clear all data
	void clear()
	{
		id = 0;
		name.clear();
		description.clear();
		created_at.clear();
		updated_at.clear();
	}
};