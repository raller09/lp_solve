unit dlgGotoLine;

interface

uses
  {$IFNDEF LCL} Windows, Messages, {$ELSE} LclIntf, LMessages, LclType, LResources, {$ENDIF}
  SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Menus;

type
  TGotoLineForm = class(TForm)
    Button1: TButton;
    Button2: TButton;
    LineNumer: TEdit;
    Label1: TLabel;
    procedure Button1Click(Sender: TObject);
    procedure LineNumerKeyPress(Sender: TObject; var Key: Char);
    procedure CheckLine;
    procedure FormCreate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  GotoLineForm: TGotoLineForm;

implementation
uses main;
{$IFNDEF LCL}
{$R *.dfm}
{$ENDIF}

procedure TGotoLineForm.Button1Click(Sender: TObject);
begin
  CheckLine;
end;

procedure TGotoLineForm.CheckLine;
var i: integer;
begin
  if TryStrToInt(LineNumer.Text, i) and
    (i <= MainForm.Editor.Lines.Count) and
    (i > 0)  then
  ModalResult := mrOK else
  MessageDlg('Invalid line number.', mtError, [mbOK], 0);
end;

procedure TGotoLineForm.LineNumerKeyPress(Sender: TObject; var Key: Char);
begin
  if Key = #13 then
  begin
    CheckLine;
    abort;
  end;
end;

procedure TGotoLineForm.FormCreate(Sender: TObject);
begin
  TMenu.Create(self); //.Active := true;
end;

initialization
{$IFDEF LCL}
{$I dlgGotoLine.lrs}  {Include form's resource file}
{$ENDIF}

end.
