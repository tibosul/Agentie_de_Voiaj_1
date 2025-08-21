#pragma once
#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDateEdit>
#include <QCheckBox>

struct FilterCriteria
{
    QString destination;
    QString searchText;
    double minPrice = 0.0;
    double maxPrice = 99999.0;
    int maxDuration = 0;
    int minSeats = 0;
    QString sortBy = "price_asc";
    QDate startDate;
    QDate endDate;
    bool favoritesOnly = false;
};

class Advanced_Filter_Widget : public QGroupBox
{
    Q_OBJECT

public:
    explicit Advanced_Filter_Widget(QWidget* parent = nullptr);
    
    FilterCriteria getFilterCriteria() const;
    void clearFilters();
    void setFilterCriteria(const FilterCriteria& criteria);

signals:
    void filtersApplied(const FilterCriteria& criteria);
    void filtersCleared();
    void favoriteViewRequested();

private slots:
    void onApplyFilters();
    void onClearFilters();
    void onShowFavorites();
    void onPriceRangeChanged();

private:
    void setupUI();
    void connectSignals();
    void updatePriceLabels();

    // UI Components
    QComboBox* m_destinationCombo;
    QLineEdit* m_searchEdit;
    QLineEdit* m_priceMinEdit;
    QLineEdit* m_priceMaxEdit;
    QSlider* m_priceSlider;
    QLabel* m_priceRangeLabel;
    QComboBox* m_durationCombo;
    QComboBox* m_sortCombo;
    QComboBox* m_seatsCombo;
    QDateEdit* m_startDateEdit;
    QDateEdit* m_endDateEdit;
    QCheckBox* m_availableOnlyCheck;
    
    QPushButton* m_applyButton;
    QPushButton* m_clearButton;
    QPushButton* m_favoriteButton;
    QPushButton* m_statisticsButton;
};