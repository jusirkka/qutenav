#include "optionmodel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QColorDialog>
#include <QPalette>
#include <QScrollArea>
#include <QGroupBox>
#include "settings.h"

using namespace KV;

OptionModel::OptionModel(QObject *parent)
  : KPageModel(parent)
{
  m_icons["Symbols"] = "preferences-desktop-icons";
  m_icons["Chart Object Filter"] = "view-filter";
  m_icons["Depths & Contours"] = "draw-freehand";
  m_icons["Colors"] = "color-picker";
  m_icons["Text"] = "draw-text";

  reset();
}

void OptionModel::reset() {
  beginResetModel();

  qDeleteAll(m_categories);
  m_categories.clear();
  m_names.clear();


  addOption("Symbols",
            "simplifiedSymbols",
            "Simplified symbols",
            "Select simplified symbols instead of paperchart symbols.");

  addOption("Symbols",
            "plainBoundaries",
            "Plain boundaries",
            "Select plain instead of symbolized boundaries of areas.");

  addOption("Symbols",
            "fullLengthSectors",
            "Full sector lengths",
            "Show light sectors at their nominal visibility.");

  addOption("Chart Object Filter",
            "maxCategoryNames",
            "Category",
            "Select maximal category to display objects. "
            "Selected category includes the previous ones in the list.");

  addOption("Chart Object Filter",
            "showMeta",
            "Meta objects",
            "Show meta objects.");

  addOption("Depths & Contours",
            "safetyDepth",
            "Safety depth",
            "Soundings shallower than safety depth are highlighted.");

  addOption("Depths & Contours",
            "safetyContour",
            "Safety contour",
            "Depth contour shallower than safety contour is highlighted.");

  addOption("Depths & Contours",
            "shallowContour",
            "Shallow water contour",
            "Select shallow water contour for depth area coloring.");

  addOption("Depths & Contours",
            "deepContour",
            "Deep water contour",
            "Select deep water contour for depth area coloring.");

  addOption("Depths & Contours",
            "twoShades",
            "Two shades",
            "Use just two shades for depth area coloring.");

  addOption("Colors",
            "colorTableNames",
            "Colortable",
            "Select colortable for chart colors.");

  auto groups = Settings::instance()->textGroups();
  for (int i = 0; i < groups.size(); i++) {
    auto group = qobject_cast<TextGroup*>(groups[i]);
    addOption("Text", "", group->text(), group->description(), i);
  }

  endResetModel();
  emit layoutChanged();
}

void OptionModel::addCategory(const QString &name) {
  auto page = new QWidget;
  auto lay = new QVBoxLayout;
  page->setLayout(lay);
  auto area = new QScrollArea;
  area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  area->setWidgetResizable(true);
  area->setWidget(page);
  m_categories[name] = area;
  m_names << name;
  // qCDebug(FC) << "category" << name;

  // keep this the last item
  lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void OptionModel::checkOptions(const QModelIndex& idx) {
  auto page = idx.data(KPageModel::WidgetRole).value<QWidget*>();
  auto opts = page->findChildren<OptionWidget*>();
  for (auto opt: opts) {
    opt->setEnabled(true);
    opt->reset();
  }
}


void OptionModel::addOption(const QString& category,
                            const QString& property,
                            const QString& displayName,
                            const QString& description, int index) {

  if (!m_categories.contains(category)) {
    beginInsertRows(QModelIndex(), m_names.size(), m_names.size());
    addCategory(category);
    endInsertRows();
  }

  auto page = m_categories[category];
  auto lay = qobject_cast<QVBoxLayout*>(page->widget()->layout());
  auto w = createOptionWidget(property, displayName, description, index);

  connect(w, &OptionWidget::edited, this, [this, w] (bool yes) {
    emit edited(w, yes);
  });
  connect(w, &OptionWidget::defaulted, this, [this, w] (bool yes) {
    emit defaulted(w, yes);
  });
  lay->insertWidget(lay->count() - 1, w);
}



OptionWidget* OptionModel::createOptionWidget(const QString &prop,
                                              const QString &displayName,
                                              const QString &description, const int index) {
  if (index >= 0) {
    return new BooleanWidget(index, displayName, description);
  }

  auto settings = Settings::instance();

  const QMetaType::Type t = static_cast<QMetaType::Type>(settings->property(prop.toUtf8()).type());

  switch (t) {
  case QMetaType::QStringList: return new StringComboWidget(prop, displayName, description);
  case QMetaType::Double: return new RealValueWidget(prop, displayName, description);
  case QMetaType::Bool: return new BooleanWidget(prop, displayName, description);
  default: return nullptr;
  }
}

// OptionModel: ItemModel implementation

QModelIndex OptionModel::index(int row, int column, const QModelIndex &parent) const {
  if (parent.isValid()) return QModelIndex();
  return createIndex(row, column);
}

QModelIndex OptionModel::parent(const QModelIndex &/*child*/) const {
  return QModelIndex();
}

int OptionModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) return 0;
  return m_names.size();
}

int OptionModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) return 0;
  return 1;
}

QVariant OptionModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (role == Qt::DisplayRole) return m_names[index.row()];
  if (role == KPageModel::HeaderRole) return m_names[index.row()];
  if (role == KPageModel::HeaderVisibleRole) return true;

  if (role == Qt::DecorationRole && m_icons.contains(m_names[index.row()])) {
    return QIcon::fromTheme(m_icons[m_names[index.row()]]);
  }

  if (role == KPageModel::WidgetRole) {
    return QVariant::fromValue(m_categories[m_names[index.row()]]);
  }
  return QVariant();
}

// Categories filter implementation

CategoriesFilter::CategoriesFilter(QObject *parent)
  : QSortFilterProxyModel(parent) {}

bool CategoriesFilter::filterAcceptsRow(int row, const QModelIndex &parent) const {
  auto model = qobject_cast<KPageModel*>(sourceModel());
  if (model == nullptr) return false;
  auto page = model->data(model->index(row, 0, parent), KPageModel::WidgetRole).value<QWidget*>();

  auto re = filterRegularExpression();
  auto widgets = page->findChildren<OptionWidget*>();
  bool found = widgets.isEmpty();
  for (auto opt: widgets) {
    found = opt->highlight(re) || found;
  }
  return found;
}


// OptionWidget implementation

OptionWidget::OptionWidget(const QString& prop,
                           const QString& descr,
                           const QString& helpText,
                           QObject* target,
                           QWidget* parent)
  : QWidget(parent)
  , m_lay(new QHBoxLayout)
  , m_label(new QLabel)
  , m_description(descr)
  , m_property(prop)
  , m_target(target)
{

  setWhatsThis(helpText);

  m_label->setText(m_description);

  m_lay->addWidget(m_label);
  m_lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
  setLayout(m_lay);

  connect(this, &OptionWidget::edited, this, [this] (bool edits) {
    auto f = m_label->font();
    f.setItalic(edits);
    f.setBold(edits == false && defaultable());
    m_label->setFont(f);
  });
}

const QString& OptionWidget::description() const {
  return m_description;
}

bool OptionWidget::highlight(const QRegularExpression &re) {
  if (!re.isValid() || re.pattern().isEmpty()) {
    m_label->setText(m_description);
    return true;
  }
  if (m_description.contains(re)) {
    QString res;
    QRegularExpressionMatchIterator it = re.globalMatch(m_description);
    int start = 0;
    while (it.hasNext()) {
      QRegularExpressionMatch match = it.next();
      res += m_description.mid(start, match.capturedStart());
      res += QString("<span style=background-color:#ffff00;>%1</span>").arg(match.captured());
      start = match.capturedEnd();
    }
    res += m_description.mid(start);
    m_label->setText(res);
    return true;
  }
  m_label->setText(m_description);
  return false;
}

void OptionWidget::reset() {
  doReset();
  emit edited(false);
  emit defaulted(!defaultable());
}

void OptionWidget::apply() {
  if (canApply()) {
    doApply();
  }
  emit edited(false);
  emit defaulted(!defaultable());
}

void OptionWidget::defaultIt() {
  doDefault();
  doReset();
  emit edited(false);
  emit defaulted(true);
}

void OptionWidget::updateIt() {
  doUpdate();
  doReset();
  emit edited(false);
  emit defaulted(!defaultable());
}

bool OptionWidget::defaultable() const {
  return canDefault();
}


// BooleanWidget implementation

BooleanWidget::BooleanWidget(const QString& prop,
                             const QString& descr,
                             const QString& helpText,
                             QWidget* parent)
  : OptionWidget(prop, descr, helpText, Settings::instance(), parent)
  , m_box(new QCheckBox)
{
  doUpdate();
  m_lay->addWidget(m_box);
  connect(m_box, &QCheckBox::clicked, this, [this] (bool on) {
    emit edited(on != m_proxy);
  });
}

BooleanWidget::BooleanWidget(int index,
                             const QString& descr,
                             const QString& helpText,
                             QWidget* parent)
  : OptionWidget("enabled", descr, helpText, Settings::instance()->textGroups()[index], parent)
  , m_box(new QCheckBox)
{
  doUpdate();
  m_lay->addWidget(m_box);
  connect(m_box, &QCheckBox::clicked, this, [this] (bool on) {
    emit edited(on != m_proxy);
  });
}

void BooleanWidget::doReset() {
  m_box->setChecked(m_proxy);
}

void BooleanWidget::doApply() {
  m_proxy = m_box->isChecked();
  m_target->setProperty(m_property.toUtf8(), m_proxy);
}

void BooleanWidget::doUpdate() {
  m_proxy = m_target->property(m_property.toUtf8()).toBool();
}

void BooleanWidget::doDefault() {
  auto group = qobject_cast<TextGroup*>(m_target);
  if (group != nullptr) {
    m_proxy = group->getDefault();
  } else {
    auto settings = qobject_cast<Settings*>(m_target);
    m_proxy = settings->getDefault(m_property).toBool();
  }
}

bool BooleanWidget::canApply() const {
  return m_proxy != m_box->isChecked();
}

bool BooleanWidget::canDefault() const {
  auto group = qobject_cast<TextGroup*>(m_target);
  if (group != nullptr) {
    return m_proxy != group->getDefault();
  }

  auto settings = qobject_cast<Settings*>(m_target);
  return m_proxy != settings->getDefault(m_property);
}



// RealValueWidget implementation

RealValueWidget::RealValueWidget(const QString& prop,
                                 const QString& descr,
                                 const QString& helpText,
                                 QWidget* parent)
  : OptionWidget(prop, descr, helpText, Settings::instance(), parent)
  , m_line(new QLineEdit)
{
  m_line->setValidator(new QDoubleValidator(0., 99.9, 1));
  doUpdate();
  m_lay->addWidget(m_line);
  connect(m_line, &QLineEdit::editingFinished, this, [this] () {
    emit edited(m_line->text().toDouble() != m_proxy);
  });
}

void RealValueWidget::doReset() {
  m_line->blockSignals(true);
  m_line->setText(QString::number(m_proxy));
  m_line->blockSignals(false);
}

void RealValueWidget::doApply() {
  m_proxy = m_line->text().toDouble();
  m_target->setProperty(m_property.toUtf8(), m_proxy);
}

void RealValueWidget::doUpdate() {
  m_proxy = m_target->property(m_property.toUtf8()).toDouble();
}

void RealValueWidget::doDefault() {
  auto settings = qobject_cast<Settings*>(m_target);
  m_proxy = settings->getDefault(m_property).toDouble();
}

bool RealValueWidget::canApply() const {
  bool ok;
  auto v = m_line->text().toDouble(&ok);
  if (!ok) return false;
  return  m_proxy != v;
}

bool RealValueWidget::canDefault() const {
  auto settings = qobject_cast<Settings*>(m_target);
  return m_proxy != settings->getDefault(m_property);
}

// StringComboWidget implementation

StringComboWidget::StringComboWidget(const QString& prop,
                                     const QString& descr,
                                     const QString& helpText,
                                     QWidget* parent)
  : OptionWidget(prop, descr, helpText, Settings::instance(), parent)
  , m_box(new QComboBox)
  , m_indexProperty(prop.chopped(5)) // maxCategoryNames -> maxCategory
{

  m_box->addItems(m_target->property(m_property.toUtf8()).toStringList());
  doUpdate();
  m_lay->addWidget(m_box);
  connect(m_box, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this] (int index) {
    emit edited(index != m_proxy);
  });
}

void StringComboWidget::doReset() {
  m_box->blockSignals(true);
  m_box->setCurrentIndex(m_proxy);
  m_box->blockSignals(false);
}

void StringComboWidget::doApply() {
  m_proxy = m_box->currentIndex();
  m_target->setProperty(m_indexProperty.toUtf8(), m_proxy);
}

void StringComboWidget::doUpdate() {
  m_proxy = m_target->property(m_indexProperty.toUtf8()).toUInt();
}

void StringComboWidget::doDefault() {
  auto settings = qobject_cast<Settings*>(m_target);
  m_proxy = settings->getDefault(m_indexProperty.toUtf8()).toUInt();
}

bool StringComboWidget::canApply() const {
  return m_proxy != m_box->currentIndex();
}

bool StringComboWidget::canDefault() const {
  auto settings = qobject_cast<Settings*>(m_target);
  return m_proxy != settings->getDefault(m_indexProperty);
}

