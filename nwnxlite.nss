void SQLInit() { }
 
void SQLExecDirect(string sSQL)
{
    SetLocalString(GetModule(), "NWNXLITE!SQLExecDirect", sSQL);
}
 
int SQLFetch()
{
    return StringToInt(GetLocalString(GetModule(), "NWNXLITE!SQLFetch"));
}
 
string SQLGetData(int iCol)
{
    return GetLocalString(GetModule(), "NWNXLITE!SQLGetData!" + IntToString(iCol - 1));
}

// deprecated. use SQLFetch().
int SQLFirstRow() { return SQLFetch(); }
int SQLNextRow() { return SQLFetch(); }