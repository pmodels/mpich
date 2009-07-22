/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.legends;

import java.net.URL;
import java.util.Comparator;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import javax.swing.JTable;

import viewer.common.Const;
/*
   Class to simulate a JMenuBar header editor for a JTable with String value
*/
public class OperationNumberMenu extends JPopupMenu
{
    private static final long  serialVersionUID = 2800L;

    private static String     count_order_icon_path
                              = Const.IMG_PATH
                              + "checkbox/CountOrder.gif";

    private JTable            table_view;
    private LegendTableModel  table_model;
    private int               cell_column;  // index where String.class is
    private Comparator        cell_comparator;

    public OperationNumberMenu( JTable in_table, int in_column )
    {
        super();
        table_view  = in_table;
        table_model = (LegendTableModel) table_view.getModel();
        cell_column = in_column;

        super.setLabel( table_model.getColumnName( cell_column ) );
        super.setToolTipText( table_model.getColumnToolTip( cell_column ) );
        switch ( cell_column ) {
            case LegendTableModel.COUNT_COLUMN :
                cell_comparator = LegendComparators.COUNT_ORDER;
                break;
            case LegendTableModel.INCL_RATIO_COLUMN :
                cell_comparator = LegendComparators.INCL_RATIO_ORDER;
                break;
            case LegendTableModel.EXCL_RATIO_COLUMN :
                cell_comparator = LegendComparators.EXCL_RATIO_ORDER;
                break;
            default:
                cell_comparator = LegendComparators.INDEX_ORDER;
        }
        this.addMenuItems();
    }

    private void addMenuItems()
    {
        JMenuItem  menu_item;
        URL        icon_URL;
        Icon       icon;


            icon_URL = null;
            icon_URL = getURL( count_order_icon_path );
            if ( icon_URL != null )
                icon = new ImageIcon( icon_URL );
            else
                icon = null;
            menu_item   = new JMenuItem( "9 ... 1", icon );
            menu_item.addActionListener(
            new ActionListener() {
                public void actionPerformed( ActionEvent evt )
                { table_model.reverseOrder( cell_comparator ); }
            } );
        super.add( menu_item );

            icon_URL = null;
            icon_URL = getURL( count_order_icon_path );
            if ( icon_URL != null )
                icon = new ImageIcon( icon_URL );
            else
                icon = null;
            menu_item   = new JMenuItem( "1 ... 9", icon );
            menu_item.addActionListener(
            new ActionListener() {
                public void actionPerformed( ActionEvent evt )
                { table_model.arrangeOrder( cell_comparator ); }
            } );
        super.add( menu_item );
    }

    private URL getURL( String filename )
    {
        return getClass().getResource( filename );
    }
}
