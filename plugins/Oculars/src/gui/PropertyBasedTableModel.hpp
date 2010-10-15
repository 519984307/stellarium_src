#ifndef PROPERTYBASEDTABLEMODEL_H
#define PROPERTYBASEDTABLEMODEL_H

#include <QAbstractTableModel>

class PropertyBasedTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	PropertyBasedTableModel(QObject *parent = 0);
	virtual ~PropertyBasedTableModel();

	void init(QList<QObject *>* content, QObject *model, QMap<int, QString> mappings);
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
	bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
	bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());

private:
	QList<QObject *>* content;
	QMap<int, QString> mappings;
	QObject* modelObject;
};

#endif // PROPERTYBASEDTABLEMODEL_H
