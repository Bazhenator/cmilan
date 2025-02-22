; ��������� ��� ����������� ������ Milan, ��������������� ��������� ������������.
; ����:  ����� ��������������� ����� n. 
; �����: n-� ����� ��������� F[n],
;        ��� F[0]=0, F[1]=1 � F[n]=F[n-2]+F[n-1] ��� ������ n>1.

; �����: ��������� ��������� ���������, alexander.s.gerasimov@ya.ru

; �������� Milan-��������� (� ��� �� �������������):
; begin
;     n:=read;
;     if n=0 then write(0)
;     else 
;         if n=1 then write(1)
;         else 
;             x:=0; y:=1; i:=2;
;             while i<=n do  /* ��������� �����: i<=n+1 � x=F[i-2] � y=F[i-1] */
;                 z:=x+y; x:=y; y:=z;
;                 i:=i+1
;             od;            /* i=n+1 � y=F[i-1]=F[n] */
;             write(y)
;         fi
;     fi
; end

; ����������: 
;  n ��������� �� ������ 99 (� ������ ������),
;  x - �� ������ 100,
;  y - �� ������ 101,
;  i - �� ������ 102,
;  z - �� ������ 103.
; ����� ������ ���������� �������, ����� ������ ���������� (� ������ ������) 
; ����� ���������� �� ������� ������ (� ������ ������); ���� � �������� 
; ������� ���������� ����� ���� �� �����, ��������, ������ 0, 1, 2, 3 � 4, 
; ����������� � �������� ��������� ������.
; ���������� �����, �� ������������� ���������������� � ������� ���������� SET:
; SET  99 0    ; n:=0
; SET 100 0    ; x:=0
; SET 101 1    ; y:=1
; SET 102 2    ; i:=2
; SET 103 0    ; z:=0

; ������� ���������:

 0: INPUT
 1: STORE 99   ; n:=read

; ������ "if n=0"
 2: LOAD 99    ; n
 3: PUSH 0
 4: COMPARE 0  ; if n=0
 5: JUMP_NO 9  ; ������� � else-�����, ���������������� "if n=0"
 6: PUSH 0
 7: PRINT      ; write(0)
 8: JUMP 41    ; ������� �� ������� ����� ����� "if n=0 then ... else ... fi"

; ������ "if n=1"
 9: LOAD 99    ; n
10: PUSH 1
11: COMPARE 0  ; if n=1
12: JUMP_NO 16 ; ������� � else-�����, ���������������� "if n=1"
13: PUSH 1
14: PRINT      ; write(1)
15: JUMP 41    ; ������� �� ������� ����� ����� "if n=1 then ... else ... fi"

; ������ "else ..." � "if n=1 then ... else ... fi"
16: PUSH 0
17: STORE 100  ; x:=0
18: PUSH 1
19: STORE 101  ; y:=1
20: PUSH 2
21: STORE 102  ; i:=2

; ������ ����� "while i<=n do ... od"
22: LOAD 102   ; i
23: LOAD 99    ; n
24: COMPARE 4  ; �������� ������� "i<=n"
;; ���������� ������ (��������� ���� ��� �������):  25:PRINT  26:STOP
25: JUMP_NO 39 ; ������� �� ������� ����� ����� ����� "while i<=n do ... od"
26: LOAD 100   ; x
27: LOAD 101   ; y
28: ADD        ; x+y
29: STORE 103  ; z:=x+y
30: LOAD 101   ; y
31: STORE 100  ; x:=y
32: LOAD 103   ; z
33: STORE 101  ; y:=z
34: LOAD 102   ; i
35: PUSH 1     
36: ADD        ; i+1
37: STORE 102  ; i:=i+1
38: JUMP 22    ; ������� � �������� ������� "i<=n"
; ����� ����� "while i<=n do ... od"

39: LOAD 101   ; y
40: PRINT      ; write(y)
; ����� "if n=1 then ... else ... fi"

41: STOP
