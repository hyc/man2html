#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char inbuf[BUFSIZ];
typedef struct myfp {
	struct myfp *prev;
	FILE *fp;
} myfp;

typedef int macro_f(char *txt);

typedef struct bv {
	int len;
	char *val;
} bv;

myfp *Fin;
bv infile;

typedef enum fonts { Roman, Italic, Bold, Fixed, Small } fontt;

typedef struct macro_s {
	bv name;
	macro_f *func;
} macro_t;

#define	BVC(x)	{ sizeof(x)-1, x }

macro_f title, section, bold, boldital, boldrom, ital, italbold, italrom;
macro_f rombold, romital, smbold, small;
macro_f deftab, hangpar, inpar, newpar, pardist, undent, indent, subsec;
macro_f tagpar, comment, eol, ftfont;
macro_f fixw, boldfixw, fixwbold, fixwital, fixwrom, italfixw, romfixw;
macro_f nofmt, dofmt, errpunct, noop, table, source;

#define	IS_DL	1
#define IS_UL	2
#define IS_IL	3
#define IS_NL	4

int indents[16];
int curind;
int wrap_nl;

char mysec;

#define	SIND	"ul"

macro_t macs[] = {
	{ BVC("TH"), title },
	{ BVC("SH"), section },
	{ BVC("SS"), subsec },
	{ BVC("SM"), small },
	{ BVC("SB"), smbold },
	{ BVC("BC"), boldfixw },
	{ BVC("BI"), boldital },
	{ BVC("BR"), boldrom },
	{ BVC("B"), bold },
	{ BVC("CB"), fixwbold },
	{ BVC("CI"), fixwital },
	{ BVC("CR"), fixwrom },
	{ BVC("CD"), fixwrom },
	{ BVC("CT"), italrom },
	{ BVC("C"), fixw },
	{ BVC("IB"), italbold },
	{ BVC("IC"), italfixw },
	{ BVC("IP"), inpar },
	{ BVC("IR"), italrom },
	{ BVC("IX"), noop },
	{ BVC("I"), ital },
	{ BVC("RB"), rombold },
	{ BVC("RC"), romfixw },
	{ BVC("RI"), romital },
	{ BVC("DT"), deftab },
	{ BVC("HP"), hangpar },
	{ BVC("LP"), newpar },
	{ BVC("PD"), pardist },
	{ BVC("PP"), newpar },
	{ BVC("P"), newpar },
	{ BVC("RE"), undent },
	{ BVC("RS"), indent },
	{ BVC("TP"), tagpar },
	{ BVC("EM"), italrom },
	{ BVC("ER"), errpunct },
	{ BVC("EV"), fixwrom },
	{ BVC("GT"), boldrom },
	{ BVC("KC"), boldrom },
	{ BVC("RV"), fixwrom },
	{ BVC("SC"), fixwrom },
	{ BVC("br"), eol },
	{ BVC("sp"), eol },
	{ BVC("nf"), nofmt },
	{ BVC("fi"), dofmt },
	{ BVC("ft"), ftfont },
	{ BVC("TA"), noop },
	{ BVC("TS"), table },
	{ BVC("so"), source },
	{ BVC("ds"), noop },
	{ BVC("hy"), noop },	/* hyphenation */
	{ BVC("\\\""), comment },
	{ {0,NULL}, NULL }
};

fontt fstk[2];

macro_f arg, fontx, str, chr, num, over, fsize, nobreak;

macro_t escs[] = {
/*	{ BVC("$"), arg }, */
	{ BVC("c"), nobreak },
	{ BVC("f"), fontx },
	{ BVC("*"), str },
	{ BVC("("), chr },
	{ BVC("n"), num },
	{ BVC("o"), over },
	{ BVC("s"), fsize },
	{ {0,NULL}, NULL }
};

typedef struct spchar {
	int c;
	char *txt;
} spchar;

typedef struct trchar {
	char *chr;
	char *txt;
} trchar;

spchar spccs[] = {
	{'\\', "\\"},
	{'e', "\\"},
	{'\'', "&acute;"},
	{'`', "&grave;"},
	{'-', "&minus;"},
	{'.', "."},
	{' ', "&nbsp;"},
	{'0', " "},	/* digit space */
	{'|', "&nbsp;"},	/* 1/6 em space, was &thinsp; */
	{'^', ""},
	{'&', ""},	/* zero-width space */
	{'%', "&#2010;"},	/* hyphen */
	{'t', "&#0009;"},	/* tab */
	{0, NULL}
};

trchar trccs[] = {
	{ "em", "&mdash;" },
	{ "hy", "-" },
	{ "bu", "&bull;" },
	{ "sq", "&#25a0;" },	/* black square */
	{ "ru", "_" },
	{ "14", "&frac14;"},
	{ "12", "&frac12;"},
	{ "34", "&frac34;"},
	{ "de", "&deg;" },
	{ "dg", "&dagger;" },
	{ "fm", "&prime;" },
	{ "ct", "&cent;" },
	{ "rg", "&reg;" },
	{ "co", "&copy;"},
	{ "ff", "&#fb00;" },
	{ "fi", "&#fb01;" },
	{ "fl", "&#fb02;" },
	{ "Fi", "&#fb03;" },
	{ "pl", "+" },
	{ "mi", "&minus;" },
	{ "eq", "=" },
	{ "**", "*" },
	{ "sc", "&sect;" },
	{ "aa", "&acute;" },
	{ "ga", "&grave;" },
	{ "ul", "_" },
	{ "sl", "slash" },
	{ "*a", "&alpha;" },
	{ "*b", "&beta;" },
	{ "*g", "&gamma;" },
	{ "*d", "&delta;" },
	{ "*e", "&epsilon" },
	{ "*z", "&zeta;" },
	{ "*y", "&eta;" },
	{ "*h", "&theta;" },
	{ "*i", "&iota;" },
	{ "*k", "&kappa;" },
	{ "*l", "&lambda;" },
	{ "*m", "&mu;" },
	{ "*n", "&nu;" },
	{ "*c", "&xi;" },
	{ "*o", "&omicron;" },
	{ "*p", "&pi;" },
	{ "*r", "&rho;" },
	{ "*s", "&sigma;" },
	{ "ts", "&sigmaf;" },
	{ "*t", "&tau;" },
	{ "*u", "&upsilon;" },
	{ "*f", "&phi;" },
	{ "*x", "&chi;" },
	{ "*q", "&psi;" },
	{ "*w", "&omega;" },
	{ "*A", "&Alpha;" },
	{ "*B", "&Beta;" },
	{ "*G", "&Gamma;" },
	{ "*D", "&Delta;" },
	{ "*E", "&Epsilon;" },
	{ "*Z", "&Zeta;" },
	{ "*Y", "&Eta;" },
	{ "*H", "&Theta;" },
	{ "*I", "&Iota;" },
	{ "*K", "&Kappa;" },
	{ "*L", "&Lambda;" },
	{ "*M", "&Mu;" },
	{ "*N", "&Nu;" },
	{ "*C", "&Xi;" },
	{ "*O", "&Omicron;" },
	{ "*P", "&Pi;" },
	{ "*R", "&Rho;" },
	{ "*S", "&Sigma;" },
	{ "*T", "&Tau;" },
	{ "*U", "&Upsilon;" },
	{ "*F", "&Phi;" },
	{ "*X", "&Chi;" },
	{ "*Q", "&Psi;" },
	{ "*O", "&Omega;" },
	{ "sr", "&radic;" },
	{ "rn", "&oline;" },
	{ ">=", "&ge;" },
	{ "<=", "&le;" },
	{ "==", "&equiv;" },
	{ "~=", "&cong;" },
	{ "ap", "&sim;" },
	{ "!=", "&ne;" },
	{ "->", "&rarr;" },
	{ "<-", "&larr;" },
	{ "ua", "&uarr;" },
	{ "da", "&darr;" },
	{ "mu", "&times;" },
	{ "di", "&divide;" },
	{ "+-", "&plusmn;" },
	{ "cu", "&cup;" },
	{ "ca", "&cap;" },
	{ "sb", "&sub;" },
	{ "sp", "&sup;" },
	{ "ib", "&sube;" },
	{ "ip", "&supe;" },
	{ "if", "&infin;" },
	{ "pd", "&part;" },
	{ "gr", "&nabla;" },
	{ "no", "&not;" },
	{ "is", "&int;" },
	{ "pt", "&prop;" },
	{ "es", "&empty;" },
	{ "mo", "&isin;" },
	{ "br", "&verbar;" },
	{ "dd", "&Dagger;" },
	{ "rh", "&raquo;" },	/* hand */
	{ "lh", "&laquo;" },
	{ "bs", "Bell" },
	{ "or", "&or;" },
	{ "ci", "O" },	/* circle */
	{ "lt", "(" },	/* big curly brackets */
	{ "lb", "(" },
	{ "rt", ")" },
	{ "rb", ")" },
	{ "lk", "{" },
	{ "rk", "}" },
	{ "bv", "|" },
	{ "lf", "&lfloor;" },
	{ "rf", "&rfloor;" },
	{ "lc", "&lceil;" },
	{ "rc", "&rceil;" },
	{ NULL, NULL }
};

/* Standard says up to 6 */
bv words[20];

char *fget2(char *s, int size, myfp *in)
{
	char *p;
	int c = EOF;
	int isquote = 0;

	if (!in || !in->fp)
		return NULL;

	for (p=s; p<s+size-1;)
	{
		c = getc(in->fp);
		if (isquote) {
			isquote = 0;
		} else if (c == '\\') {
			isquote = 1;
		}
		if (c == EOF || c == '\n')
		{
			if ( c == '\n' && isquote )
			{
				p--;
				continue;
			}
			*p = '\0';
			if (c == EOF) {
				myfp *tmp = in->prev;
				fclose(in->fp);
				if (tmp) {
					in->prev = tmp->prev;
					in->fp = tmp->fp;
					free(tmp);
					continue;
				} else {
					in->fp = NULL;
				}
			}
			break;
		}
		*p++ = c;
	}
	return (p > s || c == '\n') ? p : NULL;
}

int fsize(char *in)
{
	int s;
	int pl = 0;

	if (in[1] == '-')
	{
		s = in[2];
		pl = -1;
	} else if (in[1] == '+')
	{
		s = in[2];
		pl = 1;
	} else
		s = in[1];
	if (s == '0')
		printf("</font>");
	else
	printf("<font size=%c%c>", pl > 0 ? '+' : '-', s);
	return pl ? 3 : 2;
}

int str(char *in)
{
}

int noop(char *in)
{
	return 0;
}

int chr(char *in)
{
	trchar *t;

	in++;
	for (t=trccs; t->chr; t++)
	{
		if (!strncmp(t->chr, in, 2))
		{
			printf("%s", t->txt);
			break;
		}
	}
	return 3;
}

int num(char *in)
{
}

int over(char *in)
{
}

#define NUM_FNAMS 5

char *fnams[] = { NULL, "i", "b", "tt", "small" };

int fontset(fontt f)
{
	if (fstk[0])
	{
		printf("</%s>", fnams[fstk[0]]);
	}
	fstk[1] = fstk[0];
	fstk[0] = f;
	if (f)
	{
		printf("<%s>", fnams[fstk[0]]);
	}
	return 0;
}

int fontx(char *in)
{
	fontt f;
	in++;
	if (*in == 'P')
	{
		f = fstk[1];
	} else
	{
		if (*in == 'I' || *in == '2')
		{
			f = Italic;
		} else if (*in == 'B' || *in == '3')
		{
			f = Bold;
		} else if (*in == 'S' || *in == '4')
		{
			f = Fixed;
		} else
		{
			f = Roman;
		}
	}
	fontset(f);
	return 2;
}

int unfont()
{
	if (fstk[0])
	{
		printf("</%s>", fnams[fstk[0]]);
	}
	fstk[0] = 0;
}

output(bv *txt)
{
	char *ptr;
	macro_t *e;
	for (ptr = txt->val; ptr < txt->val+txt->len;ptr++)
	{
		if (!*ptr)
			break;
		if (*ptr == '\\')
		{
			spchar *c;
			for (e=escs; e->name.len; e++)
			{
				if (!strncmp(ptr+1, e->name.val, e->name.len))
				{
					ptr += e->func(ptr+1);
					break;
				}
			}
			if (e->name.len)
				continue;
			for (c=spccs; c->c; c++)
			{
				if (ptr[1] == c->c)
				{
					printf("%s", c->txt);
					ptr++;
					break;
				}
			}
			if (c->c)
				continue;
		} else if (*ptr == '<')
		{
			printf("&lt;");
			continue;
		} else if (*ptr == '>')
		{
			printf("&gt;");
			continue;
		} else if (!strncmp(ptr, "http://", 7))
		{
			char *p2 = ptr + 7;
			int len;
			while (isalpha(*p2) || isdigit(*p2) || *p2 == '.' || *p2 == '/' ||
				*p2 == '_' || *p2 == '?' || *p2 == '=' )
				p2++;
			len = p2 - ptr;
			printf("<a href=\"%.*s\">%.*s</a>",len,ptr,len,ptr);
			ptr = p2;
		}
		putchar(*ptr);
	}
}

void undent1() {
	if (indents[curind] == IS_DL)
	{
		printf("</dl>\n");
		curind--;
	} else if (indents[curind] == IS_UL)
	{
		printf("</ul>\n");
		curind--;
	}
}

void unnest() {
	for (;curind >0; curind--)
		undent1();
	curind = 0;
}

void oneline(bv *txt)
{
	macro_t *m;
	if (inbuf[0] == '.')
	{
		int nl = 0;
		/* macro */
		for (m=macs; m->name.len; m++)
		{
			if (!strncmp(inbuf+1, m->name.val, m->name.len))
			{
				/* If ret == 1, new line has already been read */
				if ((nl=m->func(inbuf+m->name.len+1)) > 0)
					m = macs-1;
				else
					break;
			}
		}
		if (!m->name.len)
			fputs(inbuf, stdout);
		if (nl == 2) putchar('\n');
	} else {
		txt->len = txt->val - inbuf;
		txt->val = inbuf;
		if (txt->len)
		{
			output(txt);
		} else
		{
	/*		unnest(); */
			printf("<p>"); /* SIND? */
		}
		putchar('\n');
	}
}

#define CMD "/usr/bin/gunzip < %s"

main(int argc, char *argv[])
{
	char *evpath;
	bv txt = {0, inbuf};
	evpath = getenv("PATH_TRANSLATED");
	Fin = malloc(sizeof(*Fin));
	Fin->prev = NULL;
	if (argc>1)
		infile.val = argv[1];
	else
		infile.val = evpath;
	if (infile.val)
	{
		char *dot;
		infile.len = strlen(infile.val);
		for (dot = infile.val+infile.len-1; dot>=infile.val;dot--)
			if ( *dot == '.' ) break;
		if (!strcmp(dot, ".gz"))
		{
			char *buf = malloc(infile.len+sizeof(CMD));
			sprintf(buf, CMD, infile.val);
			Fin->fp = popen(buf, "r");
			mysec = dot[-1];
		}
		else
		{
			mysec = dot[1];
			Fin->fp = fopen(infile.val, "r");
		}
	} else
	{
		Fin->fp = stdin;
		mysec = '0';
	}
	if (evpath)
		printf("Content-Type: text/html\n\n");
	printf("<HTML>\n<HEAD>\n");
	while(txt.val = fget2(inbuf, sizeof(inbuf), Fin))
		oneline(&txt);
	printf("</" SIND "></tbody></table></html>\n");
}

int parsewords(char *line)
{
	char *ptr;
	int esc = 0;
	int wc = 0;
	int quote = 0;

	for (ptr=line; *ptr; ptr++)
	{
		for(;isspace(*ptr);ptr++);
		if (!*ptr)
			return wc;
		words[wc].val = ptr;
		for(;*ptr;ptr++)
		{
			if (esc)
			{
				esc = 0;
				continue;
			}
			if (*ptr == '\\')
			{
				esc = 1;
				continue;
			}
			if (*ptr == '"')
			{
				if (quote ^= 1)
				{
					if (words[wc].val == ptr)
						words[wc].val++;
				} else
				{
					if (!ptr[1] || isspace(ptr[1]))
					{
						words[wc].len = ptr - words[wc].val;
						wc++;
						break;
					}
				};
				continue;
			}
			if (isspace(*ptr) && !quote)
			{
				words[wc].len = ptr - words[wc].val;
				wc++;
				break;
			}
		}
		if (!*ptr)
		{
			words[wc].len = ptr - words[wc].val;
			if (words[wc].len)
				wc++;
			break;
		}
	}
	return wc;
}

int title(char *in) {
	int wc = parsewords(in);
	printf("<title>");
	output(&words[0]);
	printf("(");
	output(&words[1]);
	printf("): ");
	output(&words[4]);
	printf("</title></head>\n");
	printf("<table>\n<thead>\n");
	printf("<tr><td>");
	output(&words[0]);
	printf("(");
	output(&words[1]);
	printf(")<td align=\"center\">");
	output(&words[4]);
	printf("<td align=\"right\">");
	output(&words[0]);
	printf("(");
	output(&words[1]);
	printf(")\n</thead>\n");

	printf("<tfoot>\n");
	printf("<tr><td>");
	output(&words[3]);
	printf("<td align=\"center\">");
	output(&words[2]);
	printf("<td align=\"right\">");
	output(&words[0]);
	printf("(");
	output(&words[1]);
	printf(")\n</tfoot>\n<tbody><tr><td colspan=\"3\"><br><br><" SIND ">\n");

	return 0;
}

int wraptext(char *in, char *beg, char *end)
{
	int wc = parsewords(in);
	printf(beg);
	wrap_nl = 1;
	if (wc)
	{
		int i;

		for (i=0; i<wc; i++)
		{
			if (i)
			{
				putchar(' ');
			}
			output(&words[i]);
		}
	} else
	{
		bv txt;
		int c;

		c = getc(Fin->fp);
		ungetc(c, Fin->fp);
		if ( c != '.' ) {
			txt.val = fget2(inbuf, sizeof(inbuf), Fin);
			txt.len = txt.val - inbuf;
			txt.val = inbuf;
			output(&txt);
		}
	}
	printf("%s", end);
	if (wrap_nl)
		putchar('\n');
	return 0;
}

int nobreak(char *in)
{
	*in = '\0';
	wrap_nl = 0;
	return 0;
}

int fontpair(char *in, fontt a, fontt b)
{
	int i, wc = parsewords(in);
	int isref = 0;

	if (wc == 2 && words[1].val[0] == '(' &&
		words[1].val[2] == ')') {
		char *c;
		isref = 1;
		printf("<a href=\"");
		if (words[1].val[1] != mysec)
			printf("../man%c/", words[1].val[1]);
		for (c=words[0].val; c<words[0].val+words[0].len; c++)
			if (*c != '\\') putchar(*c);
		printf(".%c\">", words[1].val[1]);
	}
	for (i=0; i<wc; i++)
	{
		fontset((i & 1) ? b : a);
		output(&words[i]);
		unfont();
		if (isref && !i)
			printf("</a>");

	}
	putchar('\n');
	return 0;
}

int errpunct(char *in)
{
	int i, wc = parsewords(in);
	char *f;
	for (i=0; i<wc; i++)
	{
		if (!(i&1))
		{
			putchar('[');
		}
		output(&words[i]);
		if (!(i&1))
		{
			putchar(']');
		}
	}
	putchar('\n');
}

int section(char *in) {
	unnest();
	unfont();
	return wraptext(in, "</" SIND ">\n\n<h3>", "</h3><" SIND ">");
}

int subsec(char *in) {
	unnest();
	unfont();
	return wraptext(in, "</" SIND ">\n\n<h4>", "</h4><" SIND ">");
}

int bold(char *in) {
	unfont();
	return wraptext(in, "<b>", "</b>");
}

int boldfixw(char *in) {
	return fontpair(in, Bold, Fixed);
}

int boldital(char *in) {
	return fontpair(in, Bold, Italic);
}

int boldrom(char *in) {
	return fontpair(in, Bold, Roman);
}

int fixw(char *in) {
	unfont();
	return wraptext(in, "<tt>", "</tt>");
}

int fixwbold(char *in) {
	return fontpair(in, Fixed, Bold);
}

int fixwital(char *in) {
	return fontpair(in, Fixed, Italic);
}

int fixwrom(char *in) {
	return fontpair(in, Fixed, Roman);
}

int ital(char *in) {
	unfont();
	return wraptext(in, "<i>", "</i>");
}

int italbold(char *in) {
	return fontpair(in, Italic, Bold);
}

int italfixw(char *in) {
	return fontpair(in, Italic, Fixed);
}

int italrom(char *in) {
	return fontpair(in, Italic, Roman);
}

int rombold(char *in) {
	return fontpair(in, Roman, Bold);
}

int romfixw(char *in) {
	return fontpair(in, Roman, Fixed);
}

int romital(char *in) {
	return fontpair(in, Roman, Italic);
}

int smbold(char *in) {
	return fontpair(in, Small, Bold);
}

int small(char *in) {
	unfont();
	return wraptext(in, "<small>", "</small>");
}

int deftab(char *in) {
}

int hangpar(char *in) {
	bv txt = {0, inbuf};
	undent1();
	printf("<p>\n<dl compact><dt>\n");
	curind++;
	indents[curind] = IS_DL;
	txt.val = fget2(inbuf, sizeof(inbuf), Fin);
	while (inbuf[0] == '.') {
		oneline(&txt);
		txt.val = fget2(inbuf, sizeof(inbuf), Fin);
	}
	printf("<dd>\n");
	txt.len = txt.val - inbuf;
	txt.val = inbuf;
	output(&txt);
	if (indents[curind] == IS_NL)
		curind--;
	return 0;
}

int nofmt(char *in) {
	printf("<pre>\n");
	return 0;
}

int dofmt(char *in) {
	printf("</pre>\n");
	return 0;
}

int tagpar(char *in) {
	bv txt = {0, inbuf};
	undent1();
	curind++;
	txt.val = fget2(inbuf, sizeof(inbuf), Fin);
	if (!strncmp(inbuf, "\\(bu", 4)) {
		indents[curind] = IS_UL;
		printf("<p><ul type=\"disc\"><li>\n");
	} else
	{
		indents[curind] = IS_DL;
		printf("<p>\n<dl compact><dt>\n");
		oneline(&txt);
		printf("<dd>\n");
	}
	return 0;
}

int inpar(char *in) {
	int wc = parsewords(in);
	undent1();
	curind++;
	indents[curind] = IS_DL;
	printf("<p>\n<dl><dt>\n");
	if (wc)
		output(words);
	printf("<dd>\n");
	return 0;
}

int newpar(char *in) {
	undent1();
	printf("<p>\n");
	return 0;
}

int pardist(char *in) {
	return 0;
}

int undent(char *in) {
	int wc=parsewords(in);
	int i = 0;

	if (wc)
		i = atoi(words[0].val);
	if (!i)
		i = 1;
	
	for (;i>0;i--)
	{
		/* close any current DL */
		undent1();
		if (indents[curind] == IS_IL) {
			printf("</ul>\n");
		}
		curind--;
		if (curind <=0)
		{
			curind = 0;
			break;
		}
	}
	return 0;
}

int indent(char *in) {
	int id = IS_IL;

	if (indents[curind] != IS_DL)
		printf("<ul>\n");
	else
		id = IS_NL;
	curind++;
	indents[curind] = id;
	return 0;
}

int comment(char *in) {
	int rc = 2;
	char *delim = "<!--";
	do {
		printf("%s%s", delim, in);
		if (!fget2(inbuf, sizeof(inbuf), Fin))
		{
			rc = 0;
			break;
		}
		delim = "\n";
	} while (!strncmp(inbuf, ".\\\"", 3));
	printf("-->\n");
	return rc;
}

int eol(char *in) {
	printf("<br>\n");
	return 0;
}

int table(char *in)
{
}

int source(char *in)
{
	myfp *fnew = malloc(sizeof(*fnew));
	char *fname;
	while (isspace(*in)) in++;
	if (infile.val) {
		char *slash;
		int len;
		slash=infile.val+infile.len-1;
		while (*slash != '/') slash--;
		len = slash - infile.val;
		fname = malloc(len+strlen(in)+2);
		strncpy(fname, infile.val, len);
		fname[len]='/';
		strcpy(fname+len+1,in);
	} else {
		fname = in;
	}
	fnew->fp = fopen(fname, "r");
	if (fname != in)
		free(fname);
	if (fnew->fp) {
		fnew->prev = Fin;
		Fin = fnew;
	} else {
		free(fnew);
	}
	return 0;
}

int ftfont(char *in) {
	int wc = parsewords(in);
	if (wc) {
		int i;

		for (i=0;i<NUM_FNAMS;i++) {
			if (fnams[i] && !strcmp(fnams[i], words[0].val)) {
				fontset(i);
				break;
			}
		}
	} else {
		unfont();
	}
	return 0;
}
