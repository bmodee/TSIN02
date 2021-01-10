import math
from tabulate import tabulate
import socket, struct
import ipaddress
import netaddr

# pip install tabulate
# pip install netaddr

#===================================================
# Optical Fiber
#===================================================

def dbm_to_mw(dbm : int):
    res = 10**(dbm/10)
    print("dBm = ", dbm, "  (given)")
    print("mW =  ", res)
    return res

def db_to_mw(db : int):
    res = (10**(db/10))/1000
    print("dB = ", db, "  (given)")
    print("mW =  ", res)
    return res

def mw_to_dbm(mw : int):
    #input must be in mW, if it is in W devision by 0.001 is needed
    res = 10*(math.log(mw, 10))
    print("mW =  ", mw, "  (given)")
    print("dBm = ", res)
    return res


def solve_p_in(p_out, alpha, length, loss, gain):
    # 1550nm -> alpha = 0.2
    # p_in -> to the cable
    p_in = p_out + alpha * length + loss - gain
    print("p_in = ", p_in, "dBm")
    return p_in

def solve_p_out(p_in, alpha, length, loss, gain):
    # p_out -> of the cable into the photodetector
    p_out = p_in - alpha * length - loss + gain
    print("p_out = ", p_out, "dBm")
    return p_out

def max_len(p_in, p_out, alpha, loss=0, gain=0):
    len = (p_in - p_out - loss + gain)/alpha
    print("maximum length = ", len, "km")
    return len

#===================================================
# Network Economics
#===================================================

def usage_based_pricing(d_p, p, k):
    # d_p, var skär grafen/linjen y - axeln
    # p, var skär grafen/linjen x - axeln
    # k, customers pays
    d_k = -k/(p/d_p) + d_p
    A = ((p-k)*d_k)/2
    B = k*d_k
    print("-------------------------------")
    print("D(k) = ", d_k)
    print("Utility = ", A+B)
    print("Cost = ", B)
    print("Net Utility (surplus) = ", A)
    if (A-D < 0):
        print("Customers and ISPs will not be happy")
    print("-------------------------------")


def flat_rate(d_p, p, k):
    # d_p, var skär grafen/linjen y - axeln
    # p, var skär grafen/linjen x - axeln
    # k, customers pays
    d_k = -k/(p/d_p) + d_p
    A = ((p-k)*d_k)/2
    B = k*d_k
    C = (k*(d_p-d_k))/2
    D = (k*(d_p-d_k))/2
    print("-------------------------------")
    print("D(k) = ", d_k)
    print("Utility = ", A+B+C)
    print("Cost = ", B+C+D)
    print("Net Utility (surplus) = ", A-D)

def compare_light_heavy_user(d_p1, d_p2, p, k):
    # d_p1, heavy user cross y-axis
    # d_p2, light user cross y-axis
    # p, lines cross x - axis
    # k, cost

    # Calculate Heavy User
    d_k1 = -k/(p/d_p1) + d_p1
    A1 = ((p-k)*d_k1)/2
    B1 = k*d_k1
    C1 = (k*(d_p1-d_k1))/2
    D1 = (k*(d_p1-d_k1))/2
    h_util = A1+B1+C1
    h_cost = B1+C1+D1
    h_surpUB = A1
    h_surpFR = A1-D1

    # Calculate Light User
    d_k2 = -k/(p/d_p2) + d_p2
    A2 = ((p-k)*d_k2)/2
    B2 = k*d_k2
    C2 = (k* (d_p1 - d_k2) ) / 2
    D2 = (k*(d_p1-d_k2))/2
    l_util = A2+B2+C2
    l_cost = B2+C2+D2
    l_surpUB = A2
    l_surpFR = A2 - ((k*(d_p2-d_k2)/2) + (k*(d_p1-d_p2)))

    print(tabulate([['Usage Based', l_surpUB, h_surpUB], ['Flate Rate', l_surpFR, h_surpFR]], headers=['Net Utility', 'Light User', 'Heavy User'], tablefmt='orgtbl'))
    return


#===================================================
# Network Economics
#===================================================
# Some shitty spaghott code, but i belive it works, at least for Some
# test cases

def allocate(req):
    suffix = 0
    res=0
    while (2**suffix < req):
        suffix=suffix+1
        res = 2**suffix
    return res

def get_suffix(req):
    suffix = 0
    res=0
    while (2**suffix < req):
        suffix=suffix+1
    return suffix


def problem4(sip : str, org1_req : int, org2_req : int):
    # This can be made so that you can have inf amount of Organisations
    # isntead of just 2, but im kinda sleepy now.


    # sip = Starting address
    alloc1 = allocate(org1_req)
    alloc2 = allocate(org2_req)

    mask1 = 32 - get_suffix(org1_req)
    mask2 = 32 - get_suffix(org2_req)
    #print(mask1, mask2)

    ip1 = ipaddress.IPv4Address(sip) + (alloc1-1) # sip addressen används också
    ip2 = ipaddress.IPv4Address(ip1) + (alloc2)

    avail_A = alloc1 - org1_req
    avail_B = alloc2 - org2_req

    print("\n==== Organisation A ====")
    print("1st IP: ", sip)
    print("|")
    print("V")
    print(str(alloc1)+"th IP: ", ip1)
    print("Available Locations left in block A: ", avail_A)

    print("\n==== Organisation B ====")
    print("1st IP: ", ip1+1)
    print("|")
    print("V")
    print(str(alloc2)+"th IP: ", ip2)
    print("Available Locations in left block B: ", avail_B)
    return

def problem5(sip : str, msk, lst : list):
    # [[businesses,addresses], [businesses,addresses]]
    # 1 st group has X businesses; each needs Y addresses
    # 2 nd group has S businesses; each needs T addresses
    # input: [ [X, Y], [S, T] ]

    #a_nice_print(lst)
    length = len(lst)
    group_start_ip = sip
    addresses_available = 2**msk
    print("====================================================")
    for i in range(length):
        print("Group: ", i+1)
        print("Businesses: ", lst[i][0])
        print("Addresses: ", lst[i][1])

        alloc = allocate(lst[i][1])
        mask = 32 - get_suffix(lst[i][1])
        ip = ipaddress.IPv4Address(group_start_ip) + (alloc) # sip addressen används också
        addresses_available = addresses_available - (lst[i][0]*lst[i][1])

        print("1st Customer: \n ", group_start_ip, "/", mask, " <to> ", ip-1, "/", mask)
        print("2nd Customer: \n ", ip, "/", mask, " <to> ", ip+alloc-1, "/", mask)
        print(str(lst[i][0])+"th Customer: \n ", ip+(alloc * (lst[i][0]-2)) , "/", mask, " <to> ", ip+((alloc) * (lst[i][0]-1))-1 , "/", mask)
        group_start_ip = ip+((alloc) * (lst[i][0]-1))
        print("====================================================")

    print("Available addresses: ", addresses_available)
    print("====================================================")
    return

def a_nice_print(lst):
    length = len(lst)
    for i in range(length):
        print("Group: ", i+1)
        print("Businesses: ", lst[i][0])
        print("Addresses: ", lst[i][1])
        print("====================================================")
    return


def ipv4_adder():
    print("adder")
    return

def ip_to_bin(ip : str):
    return ' ' .join(format(int(x), '08b') for x in ip.split('.'))

def dec_to_ip(x : int):
    return str(netaddr.IPAddress(x))

def ipv4_to_ipv6(ipv4 : str):
    return IPAddress(ipv4).ipv6()

def ipv6_to_ipv4(ipv6 : str):
    return IPAddress(ipv6).ipv4()
