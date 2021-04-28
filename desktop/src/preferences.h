/* -*- coding: utf-8-unix -*-
 *
 * File: desktop/src/preferences.h
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
#pragma once

#include <QDialog>
#include <QModelIndex>

namespace Ui {
class PreferencesDialog;
}


namespace KV {

class OptionWidget;
class OptionModel;
class CategoriesFilter;

class PreferencesDialog: public QDialog {

    Q_OBJECT

public:

  PreferencesDialog(QWidget *parent = nullptr);

  void checkAndShow();

  ~PreferencesDialog();

private slots:

  void on_defaultsButton_clicked();
  void on_resetButton_clicked();
  void on_applyButton_clicked();

  void on_pageView_currentPageChanged(const QModelIndex& curr = QModelIndex(),
                                      const QModelIndex& prev = QModelIndex());
  void on_searchLine_textEdited(const QString& filter);

  void optionModel_edited(OptionWidget* opt, bool edited);
  void optionModel_defaulted(OptionWidget* opt, bool defaulted);

private:

  void updateState();

private:

  using OptionVector = QVector<OptionWidget*>;

  Ui::PreferencesDialog* m_ui;
  OptionVector m_edits;
  OptionVector m_nonDefaults;
  OptionModel* m_model;
  CategoriesFilter* m_filter;
};

}
