import filecmp
import os

g_key_table = None


class HsmParam:
    def __init__(self):
        # you can add parameter required by your HSM here
        self.m_ref_key_path = ""
        self.m_key_id = 0
        self.m_attr1 = 0
        self.m_attr2 = 0
        self.m_padding_dict = {"raw": 0, "pss": 1}
        self.m_padding = 0  # default: raw


def create_key_table():
    global g_key_table
    if g_key_table is None:
        # create key table
        # here is reference design, please customize
        # this part according to your HSM spec.
        script_folder, script_name = os.path.split(os.path.realpath(__file__))
        key_folder = os.path.join(script_folder, "keys")
        key_folder = os.path.join(key_folder, "hsm")
        g_key_table = list()
        # key1 parameter
        key1_param = HsmParam()
        key1_param.m_ref_key_path = os.path.join(key_folder, 'pubk1.pem')
        key1_param.m_key_id = 0
        key1_param.m_attr1 = 1
        key1_param.m_attr2 = 1
        g_key_table.append(key1_param)
        # key2 parameter
        key2_param = HsmParam()
        key2_param.m_ref_key_path = os.path.join(key_folder, 'pubk2.pem')
        key2_param.m_key_id = 1
        key2_param.m_attr1 = 2
        key2_param.m_attr2 = 2
        g_key_table.append(key2_param)
    return


def query_key_table(key):
    global g_key_table
    create_key_table()
    for key_table_entry in g_key_table:
        if filecmp.cmp(key, key_table_entry.m_ref_key_path):
            print "key index: " + hex(key_table_entry.m_key_id)
            return key_table_entry
    print "no valid key entry found in table"
    return None


def hsm_rsa_sign(data, key, padding, sig):
    # note that key is pubk actually, use it as index for
    # HSM parameters such as key selection
    hsm_param_obj = query_key_table(key)
    if hsm_param_obj is None:
        return -1
    hsm_param_obj.m_padding = hsm_param_obj.m_padding_dict[padding]

    print "========================"
    print "HSM parameter:"
    print "    m_key_id  = " + hex(hsm_param_obj.m_key_id)
    print "    m_padding = " + hex(hsm_param_obj.m_padding)
    print "    m_attr1   = " + hex(hsm_param_obj.m_attr1)
    print "    m_attr2   = " + hex(hsm_param_obj.m_attr2)
    print "========================"

    # place hsm request here -- start
    # create dummy sig for now
    sig_file = open(sig, 'wb')
    for i in range(0, 256):
        sig_file.write(chr(0))
    sig_file.close()
    # place hsm request here -- end
    return 0
