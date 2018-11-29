#!/user/bin/python
import nltk
from nltk.corpus import stopwords
from nltk.stem import PorterStemmer
from nltk.tokenize import sent_tokenize
from nltk.tokenize import word_tokenize
import networkx as nx
import sys
import string
import math

# input from user : text-document to be summarized
with open(sys.argv[1], 'r') as doc:  
    data1 = doc.readlines()

sentences = []

for line in data1 :
	sentences.append(line)

sent_len = len(sentences)

if(sent_len == 0) :	
	print "Wrong Input for #tag"
	sys.exit()

#sentences.pop(sent_len-1)

if( len(sentences) <= 10) :
    file_content_back = ''
    for i in range(0, len(sentences)) :
        file_content_back += sentences[i]
        
    # writing backward summary
    file_obj = open(sys.argv[1] + "_out", 'w')       
    file_obj.write(file_content_back)
    file_obj.close()        
    sys.exit()

org_sentences = []
for s in sentences :
    org_sentences.append(s)

for i in range(0,len(sentences)) :
    sentences[i] = sentences[i].replace('-',' ')
    sentences[i] = sentences[i].replace('--',' ')
    sentences[i] = sentences[i].lower()

# removing stopwords
cachedStopWords = stopwords.words('english')
# stemming
ps = PorterStemmer()

for i in range(0,len(sentences)) :
    text = ' '.join([word for word in sentences[i].split() if word not in cachedStopWords])
    stemmedtext = ' '.join(ps.stem(word) for word in text.split())
    sentences[i] = stemmedtext

nodes = []
for i in sentences :
    nodes.append(i.strip())

list1 = []
list2 = []
edge_weights = []
for i in range(1, len(nodes)) :
    temp = []
    for j in range (0, i) :
        list1 = nodes[i].split()
        list2 = nodes[j].split()
        len_list1 = int(len(list1))
        len_list2 = int(len(list2))
        list3 = [x for x in list1 if x in list2] # list of words common in two sentences 
        s = set(list3)
        len_list3 = int(len(s))
        similarity = (len_list3)/(math.log(len_list1) + math.log(len_list2))
        temp.append(similarity)
    edge_weights.append(temp)

# di-graph formation using networkx
G1 = nx.DiGraph() # di-graph for backward variant 

# add edges to the di-graphs
for i in range (1, len(nodes)) :
    for j in range(0, i) :
        if edge_weights[i-1][j] != 0 :
            G1.add_edge(i,j, weight = edge_weights[i-1][j])
        else : 
            G1.add_node(i)
            G1.add_node(j)

d = 0.85    # default value of d
page_rank_back = []
data  = 1
for i in range(0, len(nodes)) :
    page_rank_back.append([i,data])


# calculating page-rank for the backward variant
for i in range (len(nodes)-1, -1, -1) :
    sum = 0
    for j in G1.predecessors(i) :
        weight_kj = 0
        for k in G1.successors(j) :
            weight_kj += edge_weights[j-1][k] 
        #if weight_kj != 0 :
            sum += edge_weights[j-1][i] * page_rank_back[j][1] / weight_kj
    page_rank_back[i][1] = ((1 - d) + d * sum)


def takeFirst(ele) :
    return ele[0]

def takeSecond(ele) :
    return ele[1]

# sorting w.r.t. page-rank values in non-increasing order
page_rank_back.sort(key=takeSecond, reverse = True)

temp_back = []

# input from user : number of lines in a system generated summary
m = int(10)

for i in range(0,m) :
    temp_back.append(page_rank_back[i])

# sorting backward summary-sentences in original text order
temp_back.sort(key = takeFirst)

# backward-summary content
file_content_back = ''
for i in range(0, m) :
    j = temp_back[i][0]
    file_content_back += org_sentences[j]

# writing backward summary
file_obj = open(sys.argv[1]+"_out", 'w')       
file_obj.write(file_content_back)
file_obj.close()
