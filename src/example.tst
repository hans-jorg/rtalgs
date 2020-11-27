configuration file:
;     TASK DESCRIPTION FILE
;
;     Task set descriptions, from which tasks are instantiated.
;     Keywords are at the line's beginning, and end with ':'.
;Everything in the line after the keywords or values is ignored.  Lines
;beginning with '*' are also ignored.  No line can be longer than MAX_STRING
;characters, and no name longer than MAX_NAME_LENGTH.
;
;Please, maintain the order of the parameters in the task descriptions.
;
;
start:

test set: Article's Example

MAXTIME= 27    /* timeline's upper limit (starting at 0) */
Number of Application Tasks=3

APPLICATION TASKS DESCRIPTION:
Name       Criticality  Period      Execution_time    
Task A,       HIGH,        6,           2.
Task B,       HIGH,        8,           2.
Task C,        LOW,       12,           3.
end.
