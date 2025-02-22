#include "parser.h"
#include <sstream>


// Выполняем синтаксический разбор блока program. 
// Если во время разбора не обнаруживаем никаких ошибок, 
// то выводим последовательность команд стек-машины.
void Parser::parse()
{
	program(); 
	if(!error_) {
		codegen_->flush();
	}
}

void Parser::program()
{
	mustBe(T_BEGIN);
	statementList();
	mustBe(T_END);
	codegen_->emit(STOP);
}

void Parser::statementList()
{
	// Если список операторов пуст, очередной лексемой будет одна из 
	// возможных "закрывающих скобок": END, OD, ELSE, FI.
	// В этом случае результатом разбора будет пустой блок 
	// (его список операторов равен null).
	// Если очередная лексема не входит в этот список, то ее мы считаем 
	// началом оператора и вызываем метод statement. 
	// Признаком последнего оператора является отсутствие после оператора 
	// точки с запятой.
	if(see(T_END) || see(T_OD) || see(T_ELSE) || see(T_FI)) {
		return;
	}
	else {
		bool more = true;
		while(more) {
			statement();
			more = match(T_SEMICOLON);
		}
	}
}

void Parser::statement()
{
	// Если встречаем переменную, то запоминаем ее адрес или добавляем 
	// новую если не встретили. 
	// Следующей лексемой должно быть присваивание. 
	// Затем идет блок expression, который возвращает значение на 
	// вершину стека. Записываем это значение по адресу нашей переменной.
	if(see(T_IDENTIFIER)) {
		int varAddress = findOrAddVariable(scanner_->getStringValue());
		next();
		mustBe(T_ASSIGN);
		expression();
		codegen_->emit(STORE, varAddress);
	}
	// Если встретили IF, то затем должно следовать условие. 
	// На вершине стека лежит 1 или 0 в зависимости от выполнения условия.
	// Затем зарезервируем место для условного перехода JUMP_NO к 
	// блоку ELSE (переход в случае ложного условия). Адрес перехода
	// станет известным только после того, как будет сгенерирован код  
	// для блока THEN.
	else if(match(T_IF)) {
		relation();
		
		int jumpNoAddress = codegen_->reserve();

		mustBe(T_THEN);
		statementList();
		if(match(T_ELSE)) {
			// Если есть блок ELSE, то чтобы не выполнять его в 
			// случае выполнения THEN, зарезервируем место для 
			// команды JUMP в конец этого блока.
			int jumpAddress = codegen_->reserve();
			// Заполним зарезервированное место после проверки 
			// условия инструкцией перехода в начало блока ELSE.
			codegen_->emitAt(jumpNoAddress, JUMP_NO, 
					codegen_->getCurrentAddress());
			statementList();
			// Заполним второй адрес инструкцией перехода в конец 
			// условного блока ELSE.
			codegen_->emitAt(jumpAddress, JUMP, codegen_->getCurrentAddress());
		}
		else {
			// Если блок ELSE отсутствует, то в зарезервированный 
			// адрес после проверки условия будет записана
			// инструкция условного перехода в конец оператора 
			// IF...THEN.
			codegen_->emitAt(jumpNoAddress, JUMP_NO, 
					codegen_->getCurrentAddress());
		}
		mustBe(T_FI);
	}
	else if(match(T_WHILE)) {
		// Запоминаем адрес начала проверки условия.
		int loopBeginningAddress = codegen_->getCurrentAddress();
		relation();
		// Резервируем место под инструкцию условного перехода для 
		// выхода из цикла.
		int justAfterConditionAddress = codegen_->reserve();
		mustBe(T_DO);
		statementList();
		mustBe(T_OD);
		// переходим по адресу проверки условия
		codegen_->emit(JUMP, loopBeginningAddress);
		// заполняем зарезервированный адрес инструкцией условного 
		// перехода на следующий за циклом оператор
		codegen_->emitAt(justAfterConditionAddress, JUMP_NO, 
				codegen_->getCurrentAddress());
	}
	else if(match(T_WRITE)) {
		mustBe(T_LPAREN);
		expression();
		mustBe(T_RPAREN);
		codegen_->emit(PRINT);
	}
	else if (see(T_LOOP)) {
	  // Запоминаем номер строки, на которой находится loop.
	  int loopStr = scanner_->getLineNumber();
	  codegen_->emit(PUSH, loopStr);
	  codegen_->emit(PRINT);
	  next();

	  // Считываем количество итераций тела цикла.
	  int iterationsAmount =
		findOrAddVariable(codegen_->getCurrentAddress()
		  + "iterationsAmount");
	  codegen_->emit(INPUT);
	  codegen_->emit(STORE, iterationsAmount);

	  // Запоминаем адрес начала выполнения тела цикла.
	  int loopBeginningAddress = codegen_->getCurrentAddress();

	  // Проверяем условие выполнение тела цикла 
	  // iterationsAmount <= 0.

	  codegen_->emit(LOAD, iterationsAmount);
	  codegen_->emit(PUSH, 0);
	  codegen_->emit(COMPARE, 4);

	  // Резервируем место под инструкцию условного перехода
	  // для выхода из цикла.
	  int jumpExitAddress = codegen_->reserve();

	  // Выполняем список операторов.
	  statementList();

	  // Уменьшаем количество оставшихся итераций.
	  codegen_->emit(LOAD, iterationsAmount);
	  codegen_->emit(PUSH, 1);
	  codegen_->emit(SUB);
	  codegen_->emit(STORE, iterationsAmount);

	  // Проверяем наличие лексемы endloop.
	  mustBe(T_ENDLOOP);

	  // Переходим на адрес проверки условия.
	  codegen_->emit(JUMP, loopBeginningAddress);

	  // Заполняем зарезервированный адрес инструкцией условного
	  // перехода на следующий за циклом оператор.
	  int exitStr = codegen_->getCurrentAddress();
	  codegen_->emitAt(jumpExitAddress, JUMP_YES, exitStr);
	}
	
	else if (match(T_SWITCH)) {

	  expression(); //читаем выражение; кладёт в стек значение

	  //Переменная для хранения в памяти результата выражения в switch
	  int switchValue = findOrAddVariable(codegen_->getCurrentAddress() + "SwitchValue compilation variable");
	  codegen_->emit(STORE, switchValue);

	  //Флаг, который показывает было ли выполнено хотя бы одно условие case
	  int isDefaultNeeded = findOrAddVariable(codegen_->getCurrentAddress() +  "Indicator compilation variable");

	  //Изначально значение флага 0 (подходящий case не найден)
	  codegen_->emit(PUSH, 0);
	  codegen_->emit(STORE, isDefaultNeeded);
	  int endSwitchAddress;
	  bool SwitchExit = false;
	  std::set<int> constants;

	  //Обрабатываем все встречающиеся "case"
	  do {
		if (!match(T_CASE)) break;
		if (!number(constants)) break;

		codegen_->emit(LOAD, switchValue);
		codegen_->emit(COMPARE, 0);

		int compareResult = findOrAddVariable(codegen_->getCurrentAddress() + "CompareResult compilation variable");
		codegen_->emit(STORE, compareResult);
		codegen_->emit(LOAD, compareResult);
		int skipCaseAddress = codegen_->reserve();

		//Обрабатываем содержимое "case"

		// Проверяем, что после case должна быть лексема ':'
		if (!match(T_COLON)) {
		  reportError("':' expected");
		  break;
		}

		statementList();

		// Проверяем, что после списка операторов должен быть break
		if (!match(T_BREAK)) {
		  reportError("'BREAK' expected");
		  break;
		}

		// Проверяем, что после break должна быть ';'
		if (!match(T_SEMICOLON)) {
		  reportError("';' expected");
		  break;
		}

		// Записываем в флаг, что один из кейсов подошёл
		codegen_->emit(PUSH, 1);
		codegen_->emit(STORE, isDefaultNeeded);
		codegen_->emit(LOAD, compareResult);

		SwitchExit = true;
		endSwitchAddress = codegen_->reserve();
		
		// Условный переход, если значения не равны
		codegen_->emitAt(skipCaseAddress, JUMP_NO, codegen_->getCurrentAddress());

	  } while (!match(T_DEFAULT));

	  // Проверяем, что следующая лексема должна быть ':'
	  mustBe(T_COLON);

	  //Обработка default
	  codegen_->emit(LOAD, isDefaultNeeded); //выгружаем на вершину стека
	  int skipDefaultAdress = codegen_->reserve();
	  statementList();
	  
	  // Проверяем, что после default должен быть endswitch
	  mustBe(T_ENDSWITCH);
	  
	  //Условный переход, чтобы пропустить default, если один case выполнен
	  codegen_->emitAt(skipDefaultAdress, JUMP_YES, codegen_->getCurrentAddress());

	  if (SwitchExit)
	  {
		codegen_->emitAt(endSwitchAddress, JUMP_YES, codegen_->getCurrentAddress());
	  }
	}
	else {
		reportError("statement expected.");
	}
}

void Parser::expression()
{
	/*
        Арифметическое выражение описывается следующими правилами: 
           <expression> ::= <term> {<addop> <term>}
           <addop> ::= '+' | '-'
        При разборе сначала смотрим первый терм, затем анализируем очередной 
        символ. Если это '+' или '-', то удаляем его из потока и разбираем 
        очередное слагаемое (вычитаемое). Повторяем проверку и разбор 
        очередного терма, пока не встретим за термом символ, 
        отличный от '+' и '-'.
	*/
	term();
	while(see(T_ADDOP)) {
		Arithmetic op = scanner_->getArithmeticValue();
		next();
		term();

		if(op == A_PLUS) {
			codegen_->emit(ADD);
		}
		else {
			codegen_->emit(SUB);
		}
	}
}

void Parser::term()
{
	/*  
	Терм описывается следующими правилами: 
	   <term> ::= <factor> {<mulop> <factor>}
	   <multop> ::= '*' | '/'
	При разборе сначала смотрим первый множитель, затем анализируем 
	очередной символ. Если это '*' или '/', то удаляем его из потока и 
	разбираем очередное слагаемое (вычитаемое). Повторяем проверку и 
	разбор очередного множителя, пока не встретим за ним символ, 
	отличный от '*' и '/'.
	*/
	factor();
	while(see(T_MULOP)) {
		Arithmetic op = scanner_->getArithmeticValue();
		next();
		factor();

		if(op == A_MULTIPLY) {
			codegen_->emit(MULT);
		}
		else {
			codegen_->emit(DIV);
		}
	}
}

void Parser::factor()
{
	/*
	Множитель описывается следующими правилами:
           <factor> ::= <number> | <ident> | (<expression>) | 'READ'
	*/
	if(see(T_NUMBER) || 
			see(T_ADDOP) && scanner_->getArithmeticValue() == A_MINUS) {
		// Встретили целое число без знака или 
		// знак "-" (за которым должно стоять такое число).
		bool unaryMinus = false;
		if(match(T_ADDOP)) {
			unaryMinus = true;
			if(!see(T_NUMBER)) reportError("number expected.");
		}
		
		// Записываем на вершину стека целое беззнаковое число.
		int value = scanner_->getIntValue();
		next();
		codegen_->emit(PUSH, value);
		
		// Если перед числом стоял "-", то это число, лежащее
		// на вершине стека, заменяем на противоположное.
		if(unaryMinus) codegen_->emit(INVERT);
	}
	else if(see(T_IDENTIFIER)) {
		// Встретили переменную; выгружаем значение, 
		// лежащее по ее адресу, на вершину стека.
		int varAddress = findOrAddVariable(scanner_->getStringValue());
		next();
		codegen_->emit(LOAD, varAddress);
	}
	else if(match(T_LPAREN)) {
		// Встретили открывающую скобку; следом может идти любое 
		// арифметическое выражение и обязательно закрывающая скобка.
		expression();
		mustBe(T_RPAREN);
	}
	else if(match(T_READ)) {
		// Встретили зарезервированное слово READ;
		// ему соответствует команда INPUT.
		codegen_->emit(INPUT);
	}
	else {
		reportError("expression expected.");
	}
}

void Parser::relation()
{
	// Условие сравнивает два выражения по какому-либо из знаков. 
	// Каждый знак имеет свой номер. В зависимости от результата
	// сравнения на вершине стека окажется 0 или 1.
	expression();
	if(see(T_CMP)) {
		Cmp cmp = scanner_->getCmpValue();
		next();
		expression();
		switch(cmp) {
			// для знака "=" - номер 0
			case C_EQ:
				codegen_->emit(COMPARE, 0);
				break;
			// для знака "!=" - номер 1
			case C_NE:
				codegen_->emit(COMPARE, 1);
				break;
			// для знака "<" - номер 2
			case C_LT:
				codegen_->emit(COMPARE, 2);
				break;
			// для знака ">" - номер 3
			case C_GT:
				codegen_->emit(COMPARE, 3);
				break;
			// для знака "<=" - номер 4
			case C_LE:
				codegen_->emit(COMPARE, 4);
				break;
			// для знака ">=" - номер 5
			case C_GE:
				codegen_->emit(COMPARE, 5);
				break;
		};
	}
	else {
		reportError("comparison operation expected.");
	}
}

int Parser::findOrAddVariable(const string& var)
{
	VarTable::iterator it = variables_.find(var);
	if(it == variables_.end()) {
		variables_[var] = lastVar_;
		return lastVar_++;
	}
	else {
		return it->second;
	}
}

void Parser::mustBe(Token t)
{
	if(!match(t)) {
		error_ = true;
		// Подготовим сообщение об ошибке
		std::ostringstream msg;
		msg << tokenToString(scanner_->token()) << " found while " 
			<< tokenToString(t) << " expected.";
		reportError(msg.str());
		// Попытка восстановления после ошибки
		recover(t);
	}
}

void Parser::recover(Token t)
{
	while(!see(t) && !see(T_EOF)) {
		next();
	}
	if(see(t)) {
		next();
	}
}

bool Parser::number(set<int>& s)
{
  if (see(T_NUMBER) ||
	see(T_ADDOP) && scanner_->getArithmeticValue() == A_MINUS) {
	
	// Встретили целое число без знака или
	// знак "-" (за которым должно стоять такое число).
	
	bool unaryMinus = false;
	if (match(T_ADDOP)) {
	  unaryMinus = true;
	  if (!see(T_NUMBER)) reportError("number expected.");
	}
	
	// Записываем на вершину стека целое беззнаковое число.
	int value = scanner_->getIntValue();
	
	if (!s.insert(value).second) {
	  reportError("unique constant after 'case' expected.");
	  return false;
	}
	next();
	codegen_->emit(PUSH, value);
	// Если перед числом стоял "-", то это число, лежащее
	// на вершине стека, заменяем на противоположное.
	if (unaryMinus) codegen_->emit(INVERT);
	return true;
  }
  else {
	reportError("number expected.");
	return false;
  }
}