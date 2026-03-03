#pragma once
#include <functional>
#include <string>
#include <vector>

class UndoManager
{
public:
	struct Command
	{
		std::string label;
		std::function<void()> undo;
		std::function<void()> redo;
	};

	void Push(Command command);
	void Undo();
	void Redo();
	void Clear();

	bool CanUndo() const;
	bool CanRedo() const;

private:
	std::vector<Command> m_UndoStack;
	std::vector<Command> m_RedoStack;
};

