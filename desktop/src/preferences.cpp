/* -*- coding: utf-8-unix -*-
 *
 * File: desktop/src/preferences.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "preferences.h"
#include "ui_preferencesdialog.h"
#include <KConfigGroup>
#include <KWindowConfig>
#include <KSharedConfig>
#include <QWindow>
#include <QWhatsThis>
#include <QAction>
#include <QAbstractItemView>
#include <KPageWidgetModel>
#include "optionmodel.h"
#include <QDebug>


using namespace KV;

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::PreferencesDialog)
    , m_model(new OptionModel)
    , m_filter(new CategoriesFilter)
{
  m_ui->setupUi(this);
  setWindowTitle(QString("%1: Preferences").arg(qAppName()));

  auto a = QWhatsThis::createAction(this);
  a->setIcon(QIcon::fromTheme("help-contextual"));
  m_ui->helpButton->setDefaultAction(a);

  m_filter->setSourceModel(m_model);
  m_ui->pageView->setModel(m_filter);

  connect(m_model, &OptionModel::edited,
          this, &PreferencesDialog::optionModel_edited);
  connect(m_model, &OptionModel::defaulted,
          this, &PreferencesDialog::optionModel_defaulted);

  create(); // ensure there's a window created
  const KConfigGroup cnf(KSharedConfig::openConfig(), "PreferencesDialog");
  KWindowConfig::restoreWindowSize(windowHandle(), cnf);
  resize(windowHandle()->size());

  on_pageView_currentPageChanged();
}

PreferencesDialog::~PreferencesDialog() {
  KConfigGroup cnf(KSharedConfig::openConfig(), "PreferencesDialog");
  KWindowConfig::saveWindowSize(windowHandle(), cnf);
  delete m_ui;
}

void PreferencesDialog::checkAndShow() {
  on_pageView_currentPageChanged();
  show();
}

void PreferencesDialog::on_searchLine_textEdited(const QString &s) {
  if (s.length() < 3) {
    m_filter->setFilterRegularExpression(QRegularExpression());
  } else {
    m_filter->setFilterRegularExpression(QRegularExpression(s, QRegularExpression::CaseInsensitiveOption));
  }
}

void PreferencesDialog::optionModel_edited(OptionWidget *opt, bool edited) {
  if (edited && !m_edits.contains(opt)) {
    // qDebug() << "adding" << opt->description() << "to edits";
    m_edits.append(opt);
  } else if (!edited && m_edits.contains(opt)) {
    // qDebug() << "removing" << opt->description() << "from edits";
    m_edits.removeAll(opt);
  }
  updateState();
}

void PreferencesDialog::optionModel_defaulted(OptionWidget *opt, bool defaulted) {
  if (defaulted && m_nonDefaults.contains(opt)) {
    // qDebug() << "removing" << opt->description() << "from defaults";
    m_nonDefaults.removeAll(opt);
  } else if (!defaulted && !m_edits.contains(opt)) {
    // qDebug() << "adding" << opt->description() << "to defaults";
    m_nonDefaults.append(opt);
  }
  updateState();
}

void PreferencesDialog::on_pageView_currentPageChanged(const QModelIndex& curr,
                                                       const QModelIndex& prev) {
  auto idx = m_filter->mapToSource(m_ui->pageView->currentPage());
  if (!idx.isValid()) return;
  m_model->checkOptions(idx);

  if (!m_edits.isEmpty()) {
    if (prev.isValid() && curr != prev) {
      qWarning() << "applying unsaved edits";
      on_applyButton_clicked();
    }
  }
  m_edits.clear();

  m_nonDefaults.clear();
  auto page = idx.data(KPageModel::WidgetRole).value<QWidget*>();
  auto opts = page->findChildren<OptionWidget*>();
  for (auto opt: opts) {
    if (opt->defaultable()) {
      // qCDebug(FC) << "adding" << opt->description() << "to defaults";
      m_nonDefaults.append(opt);
    }
  }
  updateState();
}

void PreferencesDialog::updateState() {
  bool editing = !m_edits.isEmpty();
  bool canDefault = !m_nonDefaults.isEmpty();

  auto view = m_ui->pageView->findChild<QAbstractItemView*>();
  if (view != nullptr) {
    view->setEnabled(!editing);
  }
  m_ui->defaultsButton->setEnabled(!editing && canDefault);
  m_ui->resetButton->setEnabled(editing);
  m_ui->applyButton->setEnabled(editing);
  m_ui->closeButton->setEnabled(!editing);
}

void PreferencesDialog::on_defaultsButton_clicked() {
  while (!m_nonDefaults.isEmpty()) {
    auto opt = m_nonDefaults.takeFirst();
    opt->defaultIt();
  }
  updateState();
}

void PreferencesDialog::on_resetButton_clicked() {
  while (!m_edits.isEmpty()) {
    auto opt = m_edits.takeFirst();
    opt->reset();
  }
  updateState();
}

void PreferencesDialog::on_applyButton_clicked() {
  while (!m_edits.isEmpty()) {
    auto opt = m_edits.takeFirst();
    opt->apply();
  }
  updateState();
}


