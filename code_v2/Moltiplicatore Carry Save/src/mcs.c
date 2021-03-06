#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_OFF "\e[0m"
#define MAGENTA(text) "\e[0;35m"

#define MASK4 0x0000000F
#define MASK8 0x000000FF
#define MASK16 0x0000FFFF
#define MASK32 0xFFFFFFFF

int main (int argc, char **argv)
{
	float alim=atof(argv[5]);;
//Inizializzazione variabili
	int n = atoi(argv[2]);
	short int i, j, out_idx = 0, y_idx = 2;
	char *param[3] = {"out", "x", "y"};
	char *param2[5] = {"sum", "Cout", "Cin", "a", "b"};
	char *param3[3] = {"z", "x", "y"};
	int num_level = n*2;
	char fileInput[50]={"netlist/"};
	strcat(fileInput, argv[1]);
//Apertura file
	FILE *fp;
	fp=fopen(fileInput, "w");
	if(fp==NULL)
	{
		printf("File doesen't exist.\n");
		return-1;
	}
//Stampe netlist iniziali fisse
	fprintf(fp, "*MOLTIPLICATORE CARRY SAVE\n.option filetype=ascii\n.INCLUDE ../lib/ST65LIKE_cell_library_v2020_1.net \n.INCLUDE ../lib/16nm_HP.pm\n");
	fprintf(fp, ".PARAM ALIM=%f\n.PARAM Lmin=16n\n.PARAM Wmin=16n\n.PARAM XXX=1\n.TRAN ", alim);
	if(n==4 || n==8)
	{
		fprintf(fp, "0.1p 820p\n\n");
	}
	else
	{
		fprintf(fp, "4p 4000p\n\n");
	}
//Codice generazione netlist cartella code
//DICHIARAZIONE SOTTOCIRCUITO AND_ARRAY_SUB: [n x AND2_SUB]	
	fprintf (fp, ".subckt AND_ARRAY_SUB\t0 Vdd ");
	for( i = 0; i < 2; i++ ) 	// i < 2 e non i < 3 perchè y la stampo a parte!
	{
		for( j = 0; j < n; j++ )
		{
			fprintf(fp, "%s%d ", param[i], j);
		}
	}
	fprintf(fp, "y\tXX=1\n");
	for ( i = 0; i < n; i++)
	{
		fprintf (fp,"xand%d\t0 Vdd %s%d %s%d y\tAND2_SUB XX=XXX\n", i, param[0], i, param[1], i);
	}
	fprintf(fp, ".ends \n\n");
//DICHIARAZIONE SOTTOCIRCUITO ADD_ARRAY_SUB: [(n-1) x FA_SUB]
	fprintf (fp, ".subckt ADD_ARRAY_SUB\t0 Vdd ");
	for( i = 0; i < 5; i++ ) 
	{
		for( j = 0; j < n-1; j++)
		{
			fprintf(fp, "%s%d ", param2[i], j);
		}
	}
	fprintf(fp, "\tXX=1\n");
	for ( i = 0; i < n-1 ; i++)
	{
		fprintf(fp, "\txadd%d 0 Vdd sum%d cout%d cin%d a%d b%d  FA_SUB XX = XXX\n", i, i, i, i, i, i);
	}
	fprintf(fp, ".ends \n\n");
//DICHIARAZIONE MCS_SUB
	fprintf (fp, ".subckt MCS_SUB\t0 Vdd\t");
	for( j = 0; j < num_level; j++)
	{
		fprintf(fp, "z%d ", j);
	}
	for( i = 1; i < 3; i++ ) 
	{
		for( j = 0; j < n; j++)
		{
			fprintf(fp, "%s%d ", param3[i], j);
		}
	}
	fprintf(fp, "\tXX=1\n");
	
	//Prima AND 0
	fprintf (fp,"xand_Level0\t 0 vdd\tz%d    ", out_idx++);	
	for ( j = 0; j < n-1; j++)
	{
		fprintf (fp,"0out%d ", j+1);
	} 
	for ( j = 0; j < n; j++)
	{
		fprintf (fp,"x%d ", j);
	} 
	fprintf (fp,"y0\tAND_ARRAY_SUB XX = XXX\n");
	
	//seconda AND 1
	fprintf (fp,"xand_Level1\t 0 vdd\t");		
	for ( j = 0; j < n; j++)
	{
		fprintf (fp,"1out%d ", j);
	} 
	for ( j = 0; j < n; j++)
	{
		fprintf (fp,"x%d ", j);
	} 
	fprintf (fp,"y1\tAND_ARRAY_SUB XX = XXX\n");


	for ( i = 2; i < num_level; i++)	//Il num totale di livelli AND+ADD è pari al doppio dei bit (num_level)
	{
		// Ogni Array di somatori è composto da (Num_bit_fattore - 1) FA_SUB
		if ( i%2 == 0 )		//Livelli intermedi di ADDIZIONE, tranne l'ultimo
		{			
			fprintf (fp,"xADD_Level%d\t 0 vdd\tz%d ", i, out_idx++);	//out_idx con post increment		
			for ( j = 0; j < (n-2); j++)					// n-2 perchè il primo termine di somma è sempre Zi
			{
				fprintf (fp,"%dsum%d ", i, j+1);
			} 
			for ( j = 0; j < (n-1); j++)
			{
				fprintf (fp,"%dcout%d ", i, j);
			}
			// Connessione Cin: livello 2 sono sempre tutti HA (Cin = 0)
			if ( i == 2 )
			{
				for ( j = 0; j < (n-1); j++)
				{
					fprintf (fp,"0     ");
				}
			} else 
			{
				for ( j = 0; j < (n-1); j++)
				{
					fprintf (fp,"%dcout%d ", i-2, j );
				}
			}
			for ( j = 0; j < (n-1); j++)			
			{
				fprintf (fp,"%dout%d ", i-1, j);
			}
			if ( i == 2 )					
			{
				for ( j = 0; j < (n-1); j++)		
				{
					fprintf (fp,"%dout%d ", i-2, j+1 );
				}
			} else 
			{
				for ( j = 0; j < (n-2); j++)
				{
					fprintf (fp,"%dsum%d ", i-2, j+1 );
				}
				fprintf (fp,"%dout%d ", i-3, n-1 );
			}
			fprintf (fp,"\tADD_ARRAY_SUB XX = XXX\n");
		} else if ( (i%2 == 1) && (i < (num_level - 1)) )		//Livelli intermedi di AND
		{
			fprintf (fp,"xand_Level%d\t 0 vdd\t", i);		
			for ( j = 0; j < n; j++)
			{
				fprintf (fp,"%dout%d ", i, j);
			} 
			for ( j = 0; j < n; j++)
			{
				fprintf (fp,"x%d ", j);
			} 
			fprintf (fp,"\ty%d\tAND_ARRAY_SUB XX = XXX\n", y_idx++);
		} else if ( (i > 1) && (i >= (num_level - 1)) )			//Ultimo livello finale di ADDIZIONE
		{		
			fprintf (fp,"xADD_Level%d\t 0 vdd\tz%d ", i, out_idx++);	//out_idx con post increment		
			for ( j = 0; j < (n-2); j++)
			{
				fprintf (fp,"z%d   ", out_idx);
				out_idx++;
			} 
			for ( j = 0; j < (n-2); j++)
			{
				fprintf (fp,"%dcout%d ", i, j);
			}
			fprintf (fp,"z%d    \t", out_idx);
			// Connessione Cin: livello 2 sono sempre tutti HA
					fprintf (fp,"0     ");
			{
				for ( j = 1; j < (n-1); j++)
				{
					fprintf (fp,"%dcout%d ", i, j-1 );
				}
			}
			// Connessione Ingressi Ai e Bi
			for ( j = 0; j < (n-1); j++)		
			{
				fprintf (fp,"%dcout%d ", i-1, j);
			}
			
			for ( j = 0; j < (n-2); j++)
				{
				fprintf (fp,"%dsum%d ", i-1, j+1 );
			}
			fprintf (fp,"%dout%d ", i-2, n-1 );
			fprintf (fp,"\tADD_ARRAY_SUB XX = XXX\n");
			
			fprintf (fp,"\t");
		}
	}
	fprintf(fp, ".ends \n\n");

//Dichiarazione del componente 
	fprintf(fp, "xmcs%d 0 Vcc ",n );
	for ( i = 0; i<num_level; i++)
	{
		fprintf(fp, "x%s%d ", param[0], i);
	}
	for ( i = 0; i<n; i++)
	{
		fprintf(fp, "x%s%d ", param[1], i);
	}
	for ( i = 0; i<n; i++)
	{
		fprintf(fp, "x%s%d ", param[2], i);
	}
	fprintf(fp, "MCS_SUB XX = XXX\n");
	//FINE DICHIARAZIONE COMPONENTE

//Fine codice generazione netlist
//Conversione operandi di ingresso
	unsigned long long int a, b;
/*TEST*/unsigned long long int s;
	a = atof (argv[3]);
	b = atof (argv[4]);
	unsigned long long int mask;
	switch (n)
	{
		case 4:
			mask = MASK4;
			break;
		case 8:
			mask = MASK8;
			break;
		case 16:
			mask = MASK16;
			break;
		case 32:
			mask = MASK32;
			break;
	}
//Controllo ingressi
	if (a> pow(2, n)-1)
	{
		printf("%sWARNING%s: inserted numbers aren't representable with %d bit\n", MAGENTA(text), COLOR_OFF, n);
		a = (a&mask);
	}
	if (b>pow(2, n)-1)
	{
		printf("%sWARNING%s: inserted numbers aren't representable with %d bit\n", MAGENTA(text), COLOR_OFF, n);
		b = (b&mask);
	}
	int A_binary[n], B_binary[n];
///*TEST*/int S_binary[n+1];		//comprende anche il bit di carry
/*TEST*/for (i = n; i >= 0; i--)
/*TEST*/{	//conversione somma da decimale a binario
///*TEST*/	S_binary[i] = s%2;		
/*TEST*/	s = s/2;
/*TEST*/}
	for (i = n-1; i >= 0; i--)
	{	//conversione operandi da decimale a binario
		A_binary[i] = a%2;
		a = a/2;
		B_binary[i] = b%2;
		b = b/2;
	}
//Scrittura ingressi convertiti nella netlist
	for (i = n-1, j = 0; j < n; i--, j++)
	{	//inizializzazione valori di A sulla netlist
		fprintf(fp, "VinA%d x%s%d 0 ", i, param[1], i);
		if(A_binary[j]==1)
		{	fprintf(fp, "ALIM\n");}	
		else
		{	fprintf(fp, "0\n");}			
	}
	for (i = n-1, j = 0; j < n; i--, j++)
	{	//inizializzazione valori di B sulla netlist
		fprintf(fp, "VinB%d x%s%d 0 ", i, param[2], i);
		if(B_binary[j]==1)
		{	fprintf(fp, "ALIM\n");}	
		else
		{	fprintf(fp, "0\n");}		
	}
	fprintf(fp, "V_dd Vcc 0 ALIM\n");	//ALIMENTAZIONE E TERMINAZIONE NETLIST
//Scrittura parametri control nella netlist
	fprintf(fp, "\n.control\nrun\nlet k=length(time)-1\nprint");
	for(i=0; i<n*2; i++)
	{
		fprintf(fp, " xout%d[k]", i);
	}
	fprintf(fp, ">outputValue.txt\nquit\n.endc\n.end");	//TERMINAZIONE NETLIST
//Chiusura file
	fclose(fp);
	return 0;
}
	
