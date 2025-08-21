#include "utils/Style_Manager.h"
#include "config.h"

#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDebug>
#include <QWidget>
#include <qstyle.h>

Style_Manager::Style_Manager()
    : m_currentTheme(Config::UI::DEFAULT_THEME)
    , m_isDarkTheme(false)
{
}

Style_Manager::~Style_Manager() = default;

void Style_Manager::load_theme(const QString& theme_name)
{
    if (theme_name == m_currentTheme)
    {
        return;
    }

    apply_theme(theme_name);
}

void Style_Manager::set_dark_theme()
{
    apply_theme("dark");
}

void Style_Manager::set_light_theme()
{
    apply_theme("light");
}

void Style_Manager::toggle_theme()
{
    if (m_isDarkTheme)
    {
        set_light_theme();
    }
    else
    {
        set_dark_theme();
    }
}

QString Style_Manager::get_current_theme() const
{
    return m_currentTheme;
}

bool Style_Manager::is_dark_theme() const
{
    return m_isDarkTheme;
}

QString Style_Manager::load_style_sheet(const QString& filename)
{
    qDebug() << "Style_Manager: Attempting to open file:" << filename;
    QFile file(filename);
    
    if (!file.exists())
    {
        qWarning() << "Style_Manager: File does not exist:" << filename;
        return QString();
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Style_Manager: Failed to open stylesheet:" << filename;
        qWarning() << "Style_Manager: Error:" << file.errorString();
        return QString();
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    
    qDebug() << "Style_Manager: Successfully read" << content.length() << "characters from" << filename;
    
    return content;
}

void Style_Manager::apply_style_sheet(const QString& style_sheet)
{
    if (QApplication* app = qApp)
    {
        // Only apply if the stylesheet is different to avoid unnecessary UI updates
        if (app->styleSheet() != style_sheet)
        {
            qDebug() << "Applying new stylesheet of length:" << style_sheet.length();
            
            // Get all top-level widgets to preserve their geometry
            QWidgetList topLevelWidgets = app->topLevelWidgets();
            QList<QRect> savedGeometries;
            
            // Save current geometries
            for (QWidget* widget : topLevelWidgets)
            {
                if (widget->isVisible())
                {
                    savedGeometries.append(widget->geometry());
                }
            }
            
            // Apply new stylesheet
            app->setStyleSheet(style_sheet);
            
            // Force a style refresh without changing sizes
            for (QWidget* widget : topLevelWidgets)
            {
                if (widget->isVisible())
                {
                    widget->style()->unpolish(widget);
                    widget->style()->polish(widget);
                    widget->update();
                }
            }
            
            qDebug() << "Stylesheet applied successfully";
        }
        else
        {
            qDebug() << "Stylesheet unchanged, skipping application";
        }
    }
}

void Style_Manager::apply_theme(const QString& theme_name)
{
    QString style_sheet_path = QString(":/styles/%1_theme.qss").arg(theme_name);
    qDebug() << "Style_Manager: Attempting to load theme from path:" << style_sheet_path;
    
    QString style_sheet = load_style_sheet(style_sheet_path);

    if (!style_sheet.isEmpty())
    {
        qDebug() << "Style_Manager: Successfully loaded stylesheet, length:" << style_sheet.length();
        apply_style_sheet(style_sheet);
        m_currentTheme = theme_name;
        m_isDarkTheme = (theme_name == "dark");

        qDebug() << "Style_Manager: Applied theme:" << theme_name << "successfully";
    }
    else
    {
        qWarning() << "Style_Manager: Failed to load theme:" << theme_name << "from path:" << style_sheet_path;
        qWarning() << "Style_Manager: Check if the resource file is compiled and accessible";
    }
}