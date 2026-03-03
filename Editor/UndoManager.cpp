#include "pch.h"
#include "UndoManager.h"

void UndoManager::Push(Command command)
{
	m_UndoStack.emplace_back(command);
	m_RedoStack.clear();
}

void UndoManager::Undo()
{
	if (m_UndoStack.empty())
		return;

	Command command = std::move(m_UndoStack.back());
	m_UndoStack.pop_back();

	if (command.undo)
	{
		command.undo();
	}
	m_RedoStack.emplace_back(std::move(command));
}

void UndoManager::Redo()
{
	if (m_RedoStack.empty())
		return;

	Command command = std::move(m_RedoStack.back());
	m_RedoStack.pop_back();

	if (command.redo)
	{
		command.redo();
	}

	m_UndoStack.emplace_back(std::move(command));
}

bool UndoManager::CanUndo() const
{
	return !m_UndoStack.empty();
}

bool UndoManager::CanRedo() const
{
	return !m_RedoStack.empty();
}
