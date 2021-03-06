// Copyright (C) 2008-2010 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "SearchPanel.h"
// #include "SearchResultsModel.h"
#include "LoseFaceApp.h"
#include "dao/User.h"
#include "dao/Photo.h"
#include <QtGui>
#include <cassert>

class SearchPanel::SearchThread : public QThread
{
  QString m_term;
  QMutex m_mutex;		// Mutex between producer and consumer
  std::list<dto::User> m_users;
  bool m_done;

public:

  SearchThread(QString term, QObject* parent = NULL)
    : QThread(parent)
    , m_term(term)
    , m_done(false)
  {
  }

  void run()
  {
    dao::General* generalDao = get_general_dao();
    if (generalDao) {
      dto::User user;
      dao::User userDao(generalDao);
      dao::UserIteratorPtr userIter = userDao.getSearchIterator(m_term.toUtf8());
      while (userIter->next(user)) {
	// Add the user in the list
	{
	  QMutexLocker locker(&m_mutex);
	  m_users.push_back(user);
	}
      }
    }
  }

  bool getNextUser(dto::User& user)
  {
    QMutexLocker locker(&m_mutex);

    if (!m_users.empty()) {
      // Get the next user from the list
      user = m_users.front();

      // And remove it
      m_users.erase(m_users.begin());

      return true;
    }
    else {
      m_done = true;
      return false;
    }
  }

  bool done() const
  {
    return m_done;
  }

};

SearchPanel::SearchPanel(QWidget* parent)
  : QWidget(parent)
  , m_thread(NULL)
  , m_poll(NULL)
{
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  QLabel* titleLabel = new QLabel(tr("Search for registered users:"));
  titleLabel->setFont(QFont("Tahoma", 12, QFont::Bold));

  m_searchBox = new QLineEdit(tr(""));
  m_searchButton = new QPushButton(tr("Search"));
  connect(m_searchButton, SIGNAL(clicked()), this, SLOT(onSearchClicked()));

  QPushButton* newUser = new QPushButton(tr("New User"));
  connect(newUser, SIGNAL(clicked()), this, SLOT(onNewUserClicked()));

  setupModel();
  setupView();

  // Create the layout of the dialog
  QGridLayout* layout = new QGridLayout();
  layout->addWidget(titleLabel, 0, 0);
  layout->addWidget(m_searchBox, 1, 0);
  layout->addWidget(m_searchButton, 1, 1);
  layout->addWidget(newUser, 1, 2);
  layout->addWidget(m_usersList, 2, 0, 1, 3);
  layout->setColumnStretch(0, 100);
  layout->setColumnStretch(3, 100);
  layout->setRowStretch(2, 100);

  setLayout(layout);
}

SearchPanel::~SearchPanel()
{
  delete m_poll;
}

void SearchPanel::onSearchClicked()
{
  // Disable search button (to avoid other signal when we are already searching users)
  m_searchButton->setEnabled(false);

  // Remove all rows from the list of users (old results)
  if (m_model->rowCount())
    m_model->removeRows(0, m_model->rowCount());

  // Create the search thread
  m_thread = new SearchThread(m_searchBox->text(), this);
  m_thread->start();

  m_poll = new QTimer(this);
  m_poll->setSingleShot(false);
  m_poll->setInterval(100);
  connect(m_poll, SIGNAL(timeout()), this, SLOT(pollResults()));
  m_poll->start();
}

void SearchPanel::onNewUserClicked()
{
  emit createUser();
}

void SearchPanel::pollResults()
{
  assert(m_thread != NULL);

  // Get all users from the working thread
  dto::User user;
  int count = 0;
  while (m_thread->getNextUser(user)) {
    QList<QStandardItem*> list;
    QStandardItem* item;

    // Get the photo of the user
    {
      dao::General* generalDao = get_general_dao();
      if (generalDao) {
	// Load the first photo of the specified user
	dao::Photo pictureDao(generalDao);
	dao::PhotoIteratorPtr pictureIter = pictureDao.getIterator(user.getId());
	dto::Photo picture;
	QImage image;
	if (pictureIter->next(picture)) {
	  image = pictureDao.loadImage(picture.getId());
	}

	item = new QStandardItem(QIcon(QPixmap::fromImage(image)), user.getUserName());
	item->setEditable(false);
	list.append(item);
      }
    }

    item = new QStandardItem(user.getFirstName());
    item->setEditable(false);
    list.append(item);

    item = new QStandardItem(user.getLastName());
    item->setEditable(false);
    list.append(item);

    item = new QStandardItem(user.getEmail());
    item->setEditable(false);
    list.append(item);

    m_model->insertRow(count++, list);
  }

  // If the thread finishes, join it and stop the timer
  if (m_thread->done()) {
    m_thread->wait();
    delete m_thread;
    m_thread = NULL;

    m_poll->stop();
    delete m_poll;
    m_poll = NULL;

    m_searchButton->setEnabled(true);
  }
}

void SearchPanel::setupModel()
{
  m_model = new QStandardItemModel(0, 4, this);
  m_model->setHeaderData(0, Qt::Horizontal, tr("User"), Qt::DisplayRole);
  m_model->setHeaderData(1, Qt::Horizontal, tr("First Name"), Qt::DisplayRole);
  m_model->setHeaderData(2, Qt::Horizontal, tr("Last Name"), Qt::DisplayRole);
  m_model->setHeaderData(3, Qt::Horizontal, tr("E-mail"), Qt::DisplayRole);
}

void SearchPanel::setupView()
{
  m_usersList = new QTableView();
  m_usersList->setModel(m_model);
  m_usersList->setIconSize(QSize(32, 32));

  QItemSelectionModel* selectionModel = new QItemSelectionModel(m_model);
  m_usersList->setSelectionModel(selectionModel);
}
