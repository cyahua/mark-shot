#include "settings/settings_text_context_menu.h"

#include "ui/i18n.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextCursor>

namespace markshot::settings {
namespace {

/// @brief 向菜单追加一项编辑动作
/// @param menu 目标菜单
/// @param target 动作回调的上下文对象
/// @param label 菜单文本
/// @param shortcut 显示的快捷键
/// @param enabled 是否可用
/// @param callback 触发回调
/// @return 无返回值
template <typename Callback>
void addEditAction(QMenu &menu, QObject *target, const QString &label, const QKeySequence &shortcut, bool enabled,
                   Callback callback)
{
    QAction *action = menu.addAction(label);
    action->setShortcut(shortcut);
    action->setShortcutVisibleInContextMenu(true);
    action->setEnabled(enabled);
    QObject::connect(action, &QAction::triggered, target, callback);
}

/// @brief 显示单行输入框的编辑菜单
/// @param edit 目标输入框
/// @param pos 输入框坐标系内的位置
/// @return 无返回值
void showLineEditMenu(QLineEdit *edit, const QPoint &pos)
{
    QMenu menu(edit);
    const bool hasSelection = edit->hasSelectedText();
    const bool hasClipboard = !QApplication::clipboard()->text().isEmpty();
    // 1. 【设置】【编辑菜单】撤销与重做
    addEditAction(menu, edit, MS_TR("Undo"), QKeySequence::Undo, edit->isUndoAvailable(), [edit] { edit->undo(); });
    addEditAction(menu, edit, MS_TR("Redo"), QKeySequence::Redo, edit->isRedoAvailable(), [edit] { edit->redo(); });
    menu.addSeparator();
    // 2. 【设置】【编辑菜单】剪切、复制、粘贴与删除
    addEditAction(menu, edit, MS_TR("Cut"), QKeySequence::Cut, hasSelection, [edit] { edit->cut(); });
    addEditAction(menu, edit, MS_TR("Copy"), QKeySequence::Copy, hasSelection, [edit] { edit->copy(); });
    addEditAction(menu, edit, MS_TR("Paste"), QKeySequence::Paste, hasClipboard, [edit] { edit->paste(); });
    addEditAction(menu, edit, MS_TR("Delete"), QKeySequence(Qt::Key_Delete), hasSelection, [edit] { edit->del(); });
    menu.addSeparator();
    // 3. 【设置】【编辑菜单】全选
    addEditAction(menu, edit, MS_TR("Select All"), QKeySequence::SelectAll, !edit->text().isEmpty(),
                  [edit] { edit->selectAll(); });
    menu.exec(edit->mapToGlobal(pos));
}

/// @brief 显示多行输入框的编辑菜单
/// @param edit 目标多行输入框
/// @param pos 视口坐标系内的位置
/// @return 无返回值
void showPlainTextMenu(QPlainTextEdit *edit, const QPoint &pos)
{
    QMenu menu(edit);
    const bool hasSelection = edit->textCursor().hasSelection();
    const bool hasClipboard = !QApplication::clipboard()->text().isEmpty();
    // 1. 【设置】【编辑菜单】撤销与重做
    addEditAction(menu, edit, MS_TR("Undo"), QKeySequence::Undo, edit->document()->isUndoAvailable(),
                  [edit] { edit->undo(); });
    addEditAction(menu, edit, MS_TR("Redo"), QKeySequence::Redo, edit->document()->isRedoAvailable(),
                  [edit] { edit->redo(); });
    menu.addSeparator();
    // 2. 【设置】【编辑菜单】剪切、复制、粘贴与删除
    addEditAction(menu, edit, MS_TR("Cut"), QKeySequence::Cut, hasSelection, [edit] { edit->cut(); });
    addEditAction(menu, edit, MS_TR("Copy"), QKeySequence::Copy, hasSelection, [edit] { edit->copy(); });
    addEditAction(menu, edit, MS_TR("Paste"), QKeySequence::Paste, hasClipboard, [edit] { edit->paste(); });
    addEditAction(menu, edit, MS_TR("Delete"), QKeySequence(Qt::Key_Delete), hasSelection, [edit] {
        QTextCursor selection = edit->textCursor();
        selection.removeSelectedText();
        edit->setTextCursor(selection);
    });
    menu.addSeparator();
    // 3. 【设置】【编辑菜单】全选
    addEditAction(menu, edit, MS_TR("Select All"), QKeySequence::SelectAll, !edit->document()->isEmpty(),
                  [edit] { edit->selectAll(); });
    menu.exec(edit->viewport()->mapToGlobal(pos));
}

}  // namespace

void installLineEditContextMenu(QLineEdit *edit)
{
    if (!edit) {
        return;
    }
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QLineEdit::customContextMenuRequested, edit,
                     [edit](const QPoint &pos) { showLineEditMenu(edit, pos); });
}

void installPlainTextEditContextMenu(QPlainTextEdit *edit)
{
    if (!edit) {
        return;
    }
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QPlainTextEdit::customContextMenuRequested, edit,
                     [edit](const QPoint &pos) { showPlainTextMenu(edit, pos); });
}

}  // namespace markshot::settings
