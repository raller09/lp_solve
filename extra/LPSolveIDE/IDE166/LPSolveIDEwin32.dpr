program LPSolveIDEwin32;
{$APPTYPE CONSOLE}
// this keeps a console window for lp_solve stderr and stdout.
// can run without but a cmd window pops up on each write
// The cmd window close box bypasses the close query method  that the widow uses.
// also if the window errors with exception, it will DRWatson from its close box
// but closing the cmd window exits both without DRWason SEND request.
{$MODE DELPHI}


uses
{$IFDEF LCL}
  {$ifdef unix}cthreads,{$endif}
  Interfaces,  // this includes the LCL widgetset

{$ENDIF}
{$IFNDEF LCL} Windows, // Messages,
{$ELSE} LclIntf, LMessages, LclType, LResources, {$ENDIF}
  Forms,  Menus,
  SysUtils,  lpsolve,
  main in 'main.pas' {MainForm},
  ResultArray in 'ResultArray.pas',
  lpobject in 'lpobject.pas' ,
  dlgSearchText in 'dlgSearchText.pas' {TextSearchDialog},
  dlgReplaceText in 'dlgReplaceText.pas' {TextReplaceDialog},
  dlgConfirmReplace in 'dlgConfirmReplace.pas' {ConfirmReplaceDialog},
  dlgGotoLine in 'dlgGotoLine.pas' {GotoLineForm},
  dlgAbout in 'dlgAbout.pas' {AboutForm},
  dlgStatistics in 'dlgStatistics.pas' {StatisticsForm} ,
  LPHighlighter in 'LPHighlighter.pas',
  Params in 'Params.pas' {ParamForm}
//  SynHighlighterXML in 'SynHighlighterXML.pas', // standard in LCL
   ;
  var
  majorversion, minorversion, release, build: integer;
  lp: LP_Handle;

{$IFDEF MSWINDOWS}
//{$R *.res}
{$ENDIF}
procedure press_ret;
begin
  writeln('[return]');
  readln;
end;

procedure error;
begin
  writeln('error');
  if (lp <> 0) then
    delete_lp(lp);
  readln;
  halt(1);
end;

{$IFDEF WINDOWS}{$R LPSolveIDEwin32.rc}{$ENDIF}

begin
  writeln('LPsolveIDE5Laz ');
  writeln('output from stderr will show here - Usually from the file read parsers');
  lp_solve_version(@majorversion, @minorversion, @release, @build);
  writeln(format('lp_solve %d.%d.%d.%d dll version', [majorversion, minorversion, release, build]));
{  writeln(format('This demo will show most of the features of lp_solve %d.%d.%d.%d', [majorversion, minorversion, release, build]));
  press_ret;
  writeln('We start by creating a new problem with 4 variables and 0 constraints');
  writeln('We use: lp := make_lp(0, 4);');
  lp := make_lp(0, 4);
  if (lp = 0) then
    error;
  press_ret();
}
//(*
{$I main.lrs }
  Application.Initialize;
  Application.HelpFile := '';
  Application.CreateForm(TMainForm, MainForm);
  if (ParamCount > 0) then
    if FileExists(ParamStr(1)) then
      MainForm.OpenFile(ParamStr(1));
  Application.Run;
// *)
end.

